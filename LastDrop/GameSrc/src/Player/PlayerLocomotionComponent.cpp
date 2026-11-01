#include "GameSrc/Player/PlayerLocomotionComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Player/HoldComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Render/SkinnedMesh/SkinnedMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 速さは「毎秒」で決め、1フレーム（1ティック。GameTime.h）ぶんに直して使う（update は1ティックに1回）。
	// 歩き・走りの速さ（m/フレーム）は、アニメの足運びに合わせる（合わないと足が地面を滑って見える）。
	// 値は元アニメ（In Place にする前）が1周で進む距離から測った。
	const float kWalkSpeed      = 1.4f / GameTime::kTicksPerSecond; // Walking（素手）
	const float kRunSpeed       = 3.2f / GameTime::kTicksPerSecond; // Slow Run（素手）
	const float kRifleWalkSpeed = 1.0f / GameTime::kTicksPerSecond; // walking（Shooter Pack）: 1.36m / 1.37秒
	const float kRifleRunSpeed  = 3.1f / GameTime::kTicksPerSecond; // rifle run（Shooter Pack）: 2.30m / 0.73秒
	const float kStopSpeed      = 0.f;
	// 加速・減速（m/フレーム²）。毎秒²の値をフレーム²に直す。
	// 止まる→走るは約0.4秒、走る→止まるは約0.27秒。減速を強めにしているのは、止まりたいときに滑り続けると操作が重く感じるため。
	const float kAcceleration = 8.f / ( GameTime::kTicksPerSecond * GameTime::kTicksPerSecond );
	const float kDeceleration = 12.f / ( GameTime::kTicksPerSecond * GameTime::kTicksPerSecond );
	// アニメを切り替えるときに前後のポーズを混ぜる長さ（フレーム）。0.2秒。
	const float kCrossFadeFrames = 0.2f * GameTime::kTicksPerSecond;
	const float kMinInputLength = 0.001f; // これより短い入力（逆方向キーの同時押しなど）は静止とみなす

	// 状態ごとの決まりごとの表。状態について知りたいこと（アニメ名・速さ・歩き方・武器を持っているか）は、すべてここから引く。
	// 状態を足すときは、Animator::State に1つ足し、この表に1行足すだけでよい（状態を並べた switch や if は他に無い）。
	// アニメ名は tools/configs/player.json の extra_animations の name。
	// 武器をしまっている間は素手のアニメ、持っている間は銃を肩の高さに持つアニメ（Basic Shooter Pack）。
	// 覗く（ADS）は専用のアニメを持たず、持っているアニメのまま FirstPersonBody が腕と銃を目の前へ寄せる。
	// 先頭は、アニメが見つからない状態・表に無い状態の代わりに使う静止の行。
	const bool kBareHands = false;
	const bool kWeaponDrawn = true;

	struct StateAnimation{
		Animator::State state;
		const char* name;
		float speed;  // この状態で進む速さ（m/フレーム）
		Gait gait;    // 歩き方（足音・足の位相の引き継ぎに使う）
		bool drawn;   // 武器を持っている状態か
	};
	const StateAnimation kStateAnimations[] = {
		{ Animator::STATE_IDLE,       "idle",       kStopSpeed,      GAIT_STILL, kBareHands   },
		{ Animator::STATE_WALK,       "walk",       kWalkSpeed,      GAIT_WALK,  kBareHands   },
		{ Animator::STATE_RUN,        "run",        kRunSpeed,       GAIT_RUN,   kBareHands   },
		{ Animator::STATE_RIFLE_IDLE, "rifle_idle", kStopSpeed,      GAIT_STILL, kWeaponDrawn },
		{ Animator::STATE_RIFLE_WALK, "rifle_walk", kRifleWalkSpeed, GAIT_WALK,  kWeaponDrawn },
		{ Animator::STATE_RIFLE_RUN,  "rifle_run",  kRifleRunSpeed,  GAIT_RUN,   kWeaponDrawn },
	};

	// 状態の行。表に無い状態は、先頭（素手の静止）の行を返す。
	const StateAnimation& stateAnimation( Animator::State state ) {
		for ( const StateAnimation& entry : kStateAnimations ) {
			if ( entry.state == state ) {
				return entry;
			}
		}
		return kStateAnimations[ 0 ];
	}

	// 足を交互に出す繰り返しのアニメか。この同士の切り替え（歩き⇔走り、素手⇔銃）だけ足の位相を合わせる。
	// 4本の歩き・走りは、どれも1周の前半で左足・後半で右足が前に出る（元の FBX で確認済み）ので、
	// 「1周の何割まで進んだか」を引き継げば足の出方が揃う。
	bool isGaitCycle( Animator::State state ) {
		return stateAnimation( state ).gait != GAIT_STILL;
	}
}

PlayerLocomotionComponent::PlayerLocomotionComponent( const SkinnedMeshData* data ) :
	mModel( nullptr ),
	mMoveDirection( 0.f, 0.f, 0.f ),
	mSpeed( 0.f ) {
	ASSERT( data && "PlayerLocomotionComponent : data is null." );
	// 状態にアニメを登録する。見つからないアニメは静止のアニメで代用する
	// （レスト姿勢＝Tポーズのまま滑るより目立たない。変換し忘れはデバッグ出力で知らせる）。
	Scene::Animation idle = data->animation( kStateAnimations[ 0 ].name );
	for ( const StateAnimation& entry : kStateAnimations ) {
		Scene::Animation animation = data->animation( entry.name );
		if ( !animation ) {
			GameLib::cout << "PlayerLocomotionComponent: animation not found: " << entry.name << GameLib::endl;
			animation = idle;
		}
		mAnimator.setAnimation( entry.state, animation );
	}
}

void PlayerLocomotionComponent::start() {
	mModel = requireComponent< SkinnedModelComponent >();
	// Animator は静止状態から始まる（request は同じ状態への遷移を無視する）ので、最初のアニメはここで付ける。
	// 武器を持って始まるなら、最初から持った静止のアニメにする（1フレーム目から素手→銃のフェードを見せない）。
	mAnimator.request( chooseState( false, false, requireComponent< HoldComponent >()->isDrawn() ) );
	mModel->mesh().setAnimation( mAnimator.animation(), 0.f );
}

Animator::State PlayerLocomotionComponent::chooseState( bool moving, bool running, bool drawn ) {
	Gait gait = !moving ? GAIT_STILL : ( running ? GAIT_RUN : GAIT_WALK );
	for ( const StateAnimation& entry : kStateAnimations ) {
		if ( entry.gait == gait && entry.drawn == drawn ) {
			return entry.state;
		}
	}
	ASSERT( false && "PlayerLocomotionComponent : no state for this gait and weapon." );
	return kStateAnimations[ 0 ].state;
}

float PlayerLocomotionComponent::speedOf( Animator::State state ) {
	return stateAnimation( state ).speed;
}

bool PlayerLocomotionComponent::isCarryState( Animator::State state ) {
	// 走りのアニメ（肩の高さで構えたまま）に合わせて抱え方を足す。素手で走るときは抱える銃が無い。
	const StateAnimation& entry = stateAnimation( state );
	return entry.drawn && entry.gait == GAIT_RUN;
}

bool PlayerLocomotionComponent::readMoveDirection( const PlayerCommand& command, float cameraYaw, Vector3* direction ) const {
	// カメラの水平の前方と右（FirstPersonCamera と同じ約束: 前方はヨー0で +Z、右は 前方×上）。
	Vector3 cameraForward( Math::sin( cameraYaw ), 0.f, Math::cos( cameraYaw ) );
	Vector3 cameraRight( -Math::cos( cameraYaw ), 0.f, Math::sin( cameraYaw ) );
	direction->set( 0.f, 0.f, 0.f );
	direction->madd( cameraForward, command.forward );
	direction->madd( cameraRight, command.strafe );
	if ( direction->length() < kMinInputLength ) {
		return false;
	}
	direction->normalize(); // 斜め入力で速くならないように
	return true;
}

void PlayerLocomotionComponent::setMoveDirection( const Vector3& direction ) {
	mMoveDirection = direction;
}

void PlayerLocomotionComponent::approachSpeed( float targetSpeed ) {
	if ( mSpeed < targetSpeed ) {
		mSpeed = Math::min( targetSpeed, mSpeed + kAcceleration );
	} else {
		mSpeed = Math::max( targetSpeed, mSpeed - kDeceleration );
	}
}

Vector3 PlayerLocomotionComponent::step() const {
	Vector3 step;
	step.setMul( mMoveDirection, mSpeed );
	return step;
}

void PlayerLocomotionComponent::changeState( Animator::State next ) {
	Animator::State previous = mAnimator.state();
	if ( !mAnimator.request( next ) ) {
		return;
	}
	Scene::Animation animation = mAnimator.animation();
	// 歩き⇔走りは、前のアニメが1周の何割まで進んでいたかを引き継いで、同じ足の位置から始める。
	// それ以外（止まる⇔動く）は頭から。止まっている姿勢には「どちらの足が前か」が無いため。
	float startTime = 0.f;
	if ( isGaitCycle( previous ) && isGaitCycle( next ) ) {
		startTime = mModel->mesh().animationPhase() * animation.period();
	}
	mModel->mesh().crossFadeTo( animation, startTime, kCrossFadeFrames );
}

Gait PlayerLocomotionComponent::gait() const {
	return stateAnimation( mAnimator.state() ).gait;
}

float PlayerLocomotionComponent::gaitPhase() const {
	return mModel->mesh().animationPhase();
}

float PlayerLocomotionComponent::walkRate() const {
	// 速さの割合は銃を持った歩き（1.0m/秒）で 1。脚が壊れて遅く歩くと小さくなり、走っても 1 で止まる（AimSway が止める）。
	return mSpeed / kRifleWalkSpeed;
}
