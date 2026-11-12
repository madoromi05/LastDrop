#include "GameSrc/Player/Body/ArmPoseComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Player/Body/FirstPersonBodyComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/MatrixBlend.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 腕のポーズ（WeaponSpec::holdAnimation など）として使うアニメの時刻
	const float kHoldPoseFrame = 0.f; // 1フレーム目

	const int kNoBone = -1; // 骨が無い（findBone の答えと同じ）

	const float kNoWeight   = 0.f; // 補正の効き具合
	const float kFullWeight = 1.f;
	// 補正を効かせる／外す長さ（フレーム）。アニメを切り替えるクロスフェードと同じ 0.2 秒
	// （素手⇔銃・歩き⇔走りのポーズの切り替えに、補正が急に入ると腕や手首が跳ねて見えるため）
	const float kWeightFadeFrames = 0.2f * GameTime::kTicksPerSecond;

	// 補正の効き具合を target へ1フレームぶん近づける
	float approachWeight( float weight, float target ) {
		float step = kFullWeight / kWeightFadeFrames;
		if ( weight < target ) {
			return Math::min( target, weight + step );
		}
		return Math::max( target, weight - step );
	}
}

ArmPoseComponent::ArmPoseComponent() :
	mHolderData( nullptr ),
	mBody( nullptr ),
	mPosedGun( nullptr ),
	mPosedHandBone( kNoBone ),
	mHasHoldPose( false ),
	mHasReadyPose( false ),
	mHasReadyBodyPose( false ),
	mHoldWeight( kNoWeight ),
	mCarryWeight( kNoWeight ) {
}

void ArmPoseComponent::awake() {
	mHolderData = requireComponent< SkinnedModelComponent >()->data();
}

void ArmPoseComponent::start() {
	mBody = requireComponent< FirstPersonBodyComponent >();
	// 持つ部品があれば、両手持ちは効いた所から始める
	// 銃を手に持つのは最初の update（HoldComponent::syncWithInventory）だが、出撃には銃が要る（StashScreen）ので持つと決めてよい
	bool armed = getComponent< HoldComponent >() != nullptr;
	mHoldWeight = armed ? kFullWeight : kNoWeight;
}

void ArmPoseComponent::blendPoses( const std::vector< Matrix34 >& from, const std::vector< Matrix34 >& to, float weight, std::vector< Matrix34 >* out ) {
	ASSERT( from.size() == to.size() && out && "ArmPoseComponent::blendPoses : bad poses." );
	out->resize( from.size() );
	for ( size_t i = 0; i < from.size(); ++i ) {
		MatrixBlend::blend( from[ i ], to[ i ], weight, &( *out )[ i ] );
	}
}

AnimationPoseSampler* ArmPoseComponent::armSampler( const char* animation ) {
	// 名前ごとに 1 つ作って使い回す（自分の骨格を 1 つずつ持つので、毎回作ると重い）。見つからないアニメは sample が false を返すだけ
	std::unique_ptr< AnimationPoseSampler >& sampler = mArmSamplers[ animation ];
	if ( !sampler ) {
		sampler = std::make_unique< AnimationPoseSampler >( mHolderData, animation );
	}
	return sampler.get();
}

void ArmPoseComponent::sampleStancePoses( const GunComponent* gun, int handBone ) {
	mPosedGun = gun;
	mPosedHandBone = handBone;
	mHasHoldPose = false;
	mHasReadyPose = false;
	mHasReadyBodyPose = false;
	if ( !gun ) {
		return; // 素手（銃が無い・手の骨が無くて持てない）
	}
	// 腕のポーズを持つ銃（拳銃）は、そのアニメの 1 フレーム目を覚えておき、持っている間いつも腕をそれに差し替える（update）
	const char* holdAnimation = gun->spec().holdAnimation;
	mHasHoldPose = holdAnimation && armSampler( holdAnimation )->sample( kHoldPoseFrame, &mHoldPose );
	// 覗いていない間の構えも同じく覚える。無ければ覗いていない間も構えのまま
	const char* readyAnimation = gun->spec().readyAnimation;
	mHasReadyPose = mHasHoldPose && readyAnimation && armSampler( readyAnimation )->sample( kHoldPoseFrame, &mReadyPose );
	// その構えを作ったときの体のアニメ（ライフルの構え）。右手を、歩き・走りで腕を寄せる基準にする
	const char* readyBodyAnimation = gun->spec().readyBodyAnimation;
	mHasReadyBodyPose = mHasReadyPose && readyBodyAnimation && handBone >= 0
		&& armSampler( readyBodyAnimation )->sample( kHoldPoseFrame, &mReadyBodyPose );
}

void ArmPoseComponent::update( Animator::State next ) {
	const HoldComponent* hold = getComponent< HoldComponent >(); // 外せる部品（無ければ腕は体のアニメのまま）
	if ( !hold ) {
		return;
	}
	GunComponent* gun = hold->gun();
	// 持ち替えた（素手になった・別の銃を持った）ら、構えのポーズを取り直す（持ち替えたティックのうちに）
	if ( gun != mPosedGun ) {
		sampleStancePoses( gun, hold->handBone() );
	}
	FirstPersonBody& body = mBody->body();
	mHoldWeight = approachWeight( mHoldWeight, hold->isWeaponInHand() ? kFullWeight : kNoWeight );
	mCarryWeight = approachWeight( mCarryWeight, PlayerLocomotionComponent::isCarryState( next ) ? kFullWeight : kNoWeight );
	// リロード中は腕をアニメへ差し替える。アニメでは左手が銃を離れるので、その間は両手持ちの補正（右手首を左手へ向ける）を弱める
	bool reloadPosed = gun && gun->isReloading()
		&& armSampler( gun->reload().animationName() )->sample( gun->reload().animationFrame(), &mReloadPose );
	float reloadWeight = reloadPosed ? gun->reload().armWeight() : kNoWeight;
	if ( mHasHoldPose ) {
		// 腕のポーズを持つ銃（拳銃）：いつも腕を差し替える（手に取る・手放すのに合わせて mHoldWeight で効かせる）
		// 覗いていない間は覗いていない間の構え（リロード中はリロードのアニメ）、覗くと構えへ上げる（覗き具合で混ぜる）
		// 覗かずに撃っても腕は上げない（ライフルと同じく、構えたまま撃つ）
		// リロードのアニメは覗いていない間の構えから始まってそこで終わるので、差し替える物を替えても腕は跳ばない
		const std::vector< Matrix34 >* ready = reloadPosed ? &mReloadPose : ( mHasReadyPose ? &mReadyPose : &mHoldPose );
		float raise = hold->aimWeight();
		blendPoses( *ready, mHoldPose, raise, &mArmPose );
		// 覗いていない間の構え（とそこから作ったリロード）はライフルの止まった構えの体の上で作ってあるので、今の体のアニメの右手の動きのとおりに
		// 腕を動かす（歩き・走りでもライフルと同じ所・同じ向き）。覗くほど弱める（覗いた構えは目へ合わせる構えなので）
		const Matrix34* reference = mHasReadyBodyPose ? &mReadyBodyPose[ mPosedHandBone ] : nullptr;
		float follow = reference ? kFullWeight - raise : kNoWeight;
		body.setArmOverride( &mArmPose, mHoldWeight, follow, reference );
	} else {
		body.setArmOverride( reloadWeight > kNoWeight ? &mReloadPose : nullptr, reloadWeight, kNoWeight, nullptr );	// ライフル：体のアニメの腕のまま。リロードのアニメはライフルの体のアニメと同じ骨格で作られているので、寄せない
	}
	body.setHoldWeight( mHoldWeight * ( kFullWeight - reloadWeight ) );
	body.setCarryWeight( mCarryWeight );
	body.setAimWeight( hold->aimWeight() );
}

float ArmPoseComponent::carryWeight() const {
	return mCarryWeight;
}
