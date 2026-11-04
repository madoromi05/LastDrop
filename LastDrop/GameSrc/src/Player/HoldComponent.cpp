#include "GameSrc/Player/HoldComponent.h"
#include "GameSrc/Player/HeldItem.h"
#include "GameSrc/Player/HeldMagazine.h"
#include "GameSrc/Player/FirstPersonBodyComponent.h"
#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/LightComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Weapon/WeaponModelComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"
#include "GameSrc/Math/MatrixBlend.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 銃を持たせる手（右手の骨）。持ち方（手から見た銃の置き方）は銃の側（WeaponModelComponent::gripInHand）。
	const char* kWeaponBone = "mixamorig:RightHand";
	// 銃を支える手。どのアニメでも、この手の骨が銃の上の決まった点に来るよう右手首を曲げる（FirstPersonBody）。
	const char* kSupportHandBone = "mixamorig:LeftHand";
	// 持ち方を合わせたアニメ（銃ごと。WeaponSpec::gripCalibrationAnimation）の時刻。このポーズで左手が銃のどこにあるかを「左手の来るべき点」にする。
	// 数値で持たずにアニメから求めるのは、持ち方を合わせ直したときに一緒に合わせ直す値を増やさないため。
	const float kGripCalibrationTime = 0.f; // 1フレーム目
	// 腕のポーズ（WeaponSpec::holdAnimation）として使うアニメの時刻。
	const float kHoldPoseFrame = 0.f;       // 1フレーム目

	const int kNoBone = -1; // 骨が無い（findBone の答えと同じ）

	const float kNoWeight   = 0.f; // 補正の効き具合
	const float kFullWeight = 1.f;
	// 補正を効かせる／外す長さ（フレーム）。アニメを切り替えるクロスフェードと同じ 0.2 秒
	// （素手⇔銃・歩き⇔走りのポーズの切り替えに、補正が急に入ると腕や手首が跳ねて見えるため）。
	const float kWeightFadeFrames = 0.2f * GameTime::kTicksPerSecond;

	// 補正の効き具合を target へ1フレームぶん近づける。
	float approachWeight( float weight, float target ) {
		float step = kFullWeight / kWeightFadeFrames;
		if ( weight < target ) {
			return Math::min( target, weight + step );
		}
		return Math::max( target, weight - step );
	}

	// 持ち方を合わせたアニメで、支える手の骨が銃のどこにあるか（銃のモデル座標）を out へ書く。
	// そのアニメが無ければ false（両手持ちの補正をしない）。持ち替えるたびに1回だけ呼ぶ（骨格を1つ作って捨てる）。
	bool findSupportHandOnWeapon( const SkinnedMeshData* data, const char* animation, int hand, int supportHand, const Matrix34& grip, Vector3* out ) {
		Scene::Animation calibration = data->animation( animation );
		if ( !calibration ) {
			GameLib::cout << "HoldComponent: animation not found: " << animation << GameLib::endl;
			return false;
		}
		Scene::Tree skeleton = data->createSkeleton();
		skeleton.setAnimation( calibration );
		skeleton.updateAnimation( kGripCalibrationTime );
		std::vector< Matrix34 > pose( data->boneNumber() );
		data->computeBonePose( skeleton, &pose );

		Matrix34 toWeapon; // 骨格の根の空間 → 銃のモデル座標
		toWeapon.setMul( pose[ hand ], grip );
		toWeapon.invert();
		const Matrix34& support = pose[ supportHand ];
		toWeapon.mul( out, Vector3( support.m03, support.m13, support.m23 ) );
		return true;
	}
}

HoldComponent::HoldComponent( const std::vector< const GameObject* >& weapons ) :
	mHolderData( nullptr ),
	mHandBone( kNoBone ),
	mSupportHandBone( kNoBone ),
	mGun( nullptr ),
	mLight( nullptr ),
	mWeaponModel( nullptr ),
	mModel( nullptr ),
	mBody( nullptr ),
	mHasHoldPose( false ),
	mHasReadyPose( false ),
	mHasReadyBodyPose( false ),
	mHoldWeight( 0.f ),
	mCarryWeight( 0.f ) {
	for ( const GameObject* weapon : weapons ) {
		if ( weapon ) {
			mWeapons.emplace_back( weapon );
		}
	}
	mHoldWeight = mStance.isDrawn() ? kFullWeight : kNoWeight;
}

// HeldItem・HeldMagazine の定義が見える所で破棄する（ヘッダでは前方宣言だけ）。
HoldComponent::~HoldComponent() {
}

void HoldComponent::awake() {
	mModel = requireComponent< SkinnedModelComponent >();
	mHolderData = mModel->data();
}

void HoldComponent::start() {
	mBody = requireComponent< FirstPersonBodyComponent >();
	mHandBone = mHolderData->findBone( kWeaponBone );
	if ( mHandBone < 0 ) {
		// 骨名違いでも落ちずに素手になるだけなので、ここで知らせる。
		GameLib::cout << "HoldComponent: weapon bone not found: " << kWeaponBone << GameLib::endl;
	}
	// 支える手が見つからなければ、右手だけで持つ（左手が銃から離れることがあるだけで落ちない）。
	mSupportHandBone = mHolderData->findBone( kSupportHandBone );
	if ( mSupportHandBone < 0 ) {
		GameLib::cout << "HoldComponent: support hand bone not found: " << kSupportHandBone << GameLib::endl;
	}
}

bool HoldComponent::equip( ItemId item ) {
	GameObject* weapon = findWeapon( item );
	if ( !weapon || weapon == mHeld.get() ) {
		return false;
	}
	bindWeapon( weapon );
	return mGun != nullptr;
}

void HoldComponent::syncWithInventory( const Inventory& inventory ) {
	// 持っている銃をカバンから捨てた（カバンに無い）ら素手にする。
	if ( mGun && inventory.count( mGun->spec().item ) == 0 ) {
		bindWeapon( nullptr );
	}
	if ( mHeld.get() ) {
		return;
	}
	// 素手で、カバンに銃があれば持つ（拾った銃をすぐ使えるように。始めの拳銃もここで持つ）。
	for ( int i = 0; i < WEAPON_NUMBER; ++i ) {
		ItemId item = weaponSpec( static_cast< WeaponId >( i ) ).item;
		if ( inventory.count( item ) > 0 && equip( item ) ) {
			return;
		}
	}
}

void HoldComponent::blendPoses( const std::vector< Matrix34 >& from, const std::vector< Matrix34 >& to, float weight, std::vector< Matrix34 >* out ) {
	ASSERT( from.size() == to.size() && out && "HoldComponent::blendPoses : bad poses." );
	out->resize( from.size() );
	for ( size_t i = 0; i < from.size(); ++i ) {
		MatrixBlend::blend( from[ i ], to[ i ], weight, &( *out )[ i ] );
	}
}

AnimationPoseSampler* HoldComponent::armSampler( const char* animation ) {
	// 名前ごとに 1 つ作って使い回す（自分の骨格を 1 つずつ持つので、毎回作ると重い）。見つからないアニメは sample が false を返すだけ。
	std::unique_ptr< AnimationPoseSampler >& sampler = mArmSamplers[ animation ];
	if ( !sampler ) {
		sampler = std::make_unique< AnimationPoseSampler >( mHolderData, animation );
	}
	return sampler.get();
}

GameObject* HoldComponent::findWeapon( ItemId item ) const {
	for ( const GameObjectHandle& handle : mWeapons ) {
		GameObject* weapon = handle.get();
		const GunComponent* gun = weapon ? weapon->getComponent< GunComponent >() : nullptr;
		if ( gun && gun->spec().item == item ) {
			return weapon;
		}
	}
	return nullptr;
}

void HoldComponent::bindWeapon( GameObject* held ) {
	if ( mGun ) {
		mGun->cancelReload(); // 替えている途中の銃を手放す（弾は込めない）
	}
	mHeld = GameObjectHandle( held );
	mGun = nullptr;
	mLight = nullptr;
	mWeaponModel = nullptr;
	mHeldItem.reset();
	mMagazine.reset();
	mHasHoldPose = false;
	mHasReadyPose = false;
	mHasReadyBodyPose = false;
	FirstPersonBody& body = mBody->body();
	const WeaponModelComponent* model = held ? held->getComponent< WeaponModelComponent >() : nullptr;
	GunComponent* gun = held ? held->getComponent< GunComponent >() : nullptr;
	if ( !model || !gun || mHandBone < 0 ) {
		body.clearWeapon(); // 素手（銃が無い・手の骨が無くて持てない）
		return;
	}
	mWeaponModel = model;
	mGun = gun;
	mLight = held->getComponent< LightComponent >();
	// 腕のポーズを持つ銃（拳銃）は、そのアニメの 1 フレーム目を覚えておき、持っている間いつも腕をそれに差し替える（updateArms）。
	const char* holdAnimation = gun->spec().holdAnimation;
	mHasHoldPose = holdAnimation && armSampler( holdAnimation )->sample( kHoldPoseFrame, &mHoldPose );
	// 覗いていない間の構えも同じく覚える。無ければ覗いていない間も構えのまま。
	const char* readyAnimation = gun->spec().readyAnimation;
	mHasReadyPose = mHasHoldPose && readyAnimation && armSampler( readyAnimation )->sample( kHoldPoseFrame, &mReadyPose );
	// その構えを作ったときの体のアニメ（ライフルの構え）。右手を、歩き・走りで腕を寄せる基準にする。
	const char* readyBodyAnimation = gun->spec().readyBodyAnimation;
	mHasReadyBodyPose = mHasReadyPose && readyBodyAnimation && mHandBone >= 0
		&& armSampler( readyBodyAnimation )->sample( kHoldPoseFrame, &mReadyBodyPose );
	Matrix34 grip = model->gripInHand();
	mHeldItem = std::make_unique< HeldItem >( model->weapon(), mHandBone, grip );
	int supportHand = mSupportHandBone;
	Vector3 supportOnWeapon( 0.f, 0.f, 0.f );
	if ( supportHand >= 0 && !findSupportHandOnWeapon( mHolderData, mGun->spec().gripCalibrationAnimation, mHandBone, supportHand, grip, &supportOnWeapon ) ) {
		supportHand = kNoBone;
	}
	Vector3 weaponMin;
	Vector3 weaponMax;
	model->bounds( &weaponMin, &weaponMax );
	body.setWeapon( mHandBone, grip, model->sightEye(), supportHand, supportOnWeapon, weaponMin, weaponMax );
	// 弾倉は支える手（左手）で抜いて挿す。支える手が無ければ、弾倉は描かない。
	if ( model->magazine() && supportHand >= 0 ) {
		mMagazine = std::make_unique< HeldMagazine >( model->magazine(), supportHand );
	}
}

GunComponent* HoldComponent::gun() const {
	return mGun;
}

GunComponent* HoldComponent::heldGun( const GameObject* holder ) {
	const HoldComponent* hold = holder ? holder->getComponent< HoldComponent >() : nullptr;
	return hold ? hold->gun() : nullptr;
}

LightComponent* HoldComponent::light() const {
	return mGun ? mLight : nullptr;
}

GameObject* HoldComponent::heldObject() const {
	return mHeld.get();
}

void HoldComponent::updateStance( bool holsterPressed, bool aimHeld, bool runHeld ) {
	mStance.update( holsterPressed, aimHeld, runHeld );
}

bool HoldComponent::isDrawn() const {
	return mStance.isDrawn();
}

bool HoldComponent::isAiming() const {
	return mStance.isAiming();
}

float HoldComponent::aimWeight() const {
	return mStance.aimWeight();
}

bool HoldComponent::isWeaponInHand() const {
	return mGun && mStance.isDrawn();
}

void HoldComponent::updateReload( bool reloadPressed, Inventory* inventory ) {
	if ( !mGun ) {
		return;
	}
	if ( !mStance.isDrawn() ) {
		// しまったらやめる（替えている途中の銃は手から消えるので、弾は込めない）。
		mGun->cancelReload();
	} else if ( reloadPressed && mGun->canStartReload( *inventory ) ) {
		mGun->startReload();
		mStance.stopAiming(); // 覗いていたら銃を下ろしてから替える
	}
	mGun->updateReload( inventory );
}

void HoldComponent::updateArms( Animator::State next ) {
	FirstPersonBody& body = mBody->body();
	mHoldWeight = approachWeight( mHoldWeight, mStance.isDrawn() ? kFullWeight : kNoWeight );
	mCarryWeight = approachWeight( mCarryWeight, PlayerLocomotionComponent::isCarryState( next ) ? kFullWeight : kNoWeight );
	// リロード中は腕をアニメへ差し替える。アニメでは左手が銃を離れるので、その間は両手持ちの補正（右手首を左手へ向ける）を弱める。
	bool reloadPosed = mGun && mGun->isReloading()
		&& armSampler( mGun->reload().animationName() )->sample( mGun->reload().animationFrame(), &mReloadPose );
	float reloadWeight = reloadPosed ? mGun->reload().armWeight() : kNoWeight;
	if ( mHasHoldPose ) {
		// 腕のポーズを持つ銃（拳銃）：いつも腕を差し替える（出す・しまうのに合わせて mHoldWeight で効かせる）。
		// 覗いていない間は覗いていない間の構え（リロード中はリロードのアニメ）、覗くと構えへ上げる（覗き具合で混ぜる）。
		// 覗かずに撃っても腕は上げない（ライフルと同じく、構えたまま撃つ。PistolAnimation_Plan.txt 14 章 Q2）。
		// リロードのアニメは覗いていない間の構えから始まってそこで終わるので、差し替える物を替えても腕は跳ばない。
		const std::vector< Matrix34 >* ready = reloadPosed ? &mReloadPose : ( mHasReadyPose ? &mReadyPose : &mHoldPose );
		float raise = mStance.aimWeight();
		blendPoses( *ready, mHoldPose, raise, &mArmPose );
		// 覗いていない間の構え（とそこから作ったリロード）はライフルの止まった構えの体の上で作ってあるので、今の体のアニメの右手の動きのとおりに
		// 腕を動かす（歩き・走りでもライフルと同じ所・同じ向き）。覗くほど弱める（覗いた構えは目へ合わせる構えなので）。
		const Matrix34* reference = mHasReadyBodyPose ? &mReadyBodyPose[ mHandBone ] : nullptr;
		float follow = reference ? kFullWeight - raise : kNoWeight;
		body.setArmOverride( &mArmPose, mHoldWeight, follow, reference );
	} else {
		// ライフル：体のアニメの腕のまま。リロードのアニメはライフルの体のアニメと同じ骨格で作られているので、寄せない。
		body.setArmOverride( reloadWeight > kNoWeight ? &mReloadPose : nullptr, reloadWeight, kNoWeight, nullptr );
	}
	body.setHoldWeight( mHoldWeight * ( kFullWeight - reloadWeight ) );
	body.setCarryWeight( mCarryWeight );
	body.setAimWeight( mStance.aimWeight() );
}

void HoldComponent::takeOutMagazineIfCued() {
	if ( !mGun || mGun->reloadCueThisTick() != WeaponReload::CUE_MAG_OUT ) {
		return;
	}
	// 弾倉を抜いた瞬間の左手から見た場所を覚える（それからは左手に付いて動く）。
	Matrix34 gun;
	if ( mMagazine && weaponPlacement( &gun ) ) {
		mMagazine->takeOut( mModel->mesh(), gun );
	}
}

float HoldComponent::carryWeight() const {
	return mCarryWeight;
}

bool HoldComponent::muzzle( Vector3* position, Vector3* direction ) const {
	if ( !isWeaponInHand() ) {
		return false;
	}
	Matrix34 gun;
	if ( !weaponPlacement( &gun ) ) {
		return false; // まだポーズが無い
	}
	mWeaponModel->muzzle( gun, position, direction );
	return true;
}

void HoldComponent::draw() const {
	if ( !mHeldItem || !mStance.isDrawn() ) {
		return;
	}
	mHeldItem->draw( mModel->mesh() );
	Matrix34 gun;
	if ( mMagazine && mGun && weaponPlacement( &gun ) ) {
		mMagazine->draw( mModel->mesh(), gun, mGun->reload().isMagazineOut(), mGun->reload().magazineSeatWeight() );
	}
}

bool HoldComponent::weaponPlacement( Matrix34* out ) const {
	return mHeldItem && mHeldItem->placement( mModel->mesh(), out );
}
