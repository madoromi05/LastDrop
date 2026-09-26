#include "PlayerCharacter.h"
#include "SkinnedMeshData.h"
#include "SkinnedMeshInstance.h"
#include "HeldItem.h"
#include "Pad.h"

#include "GameLib/GameLib.h"
#include "GameLib/Base/DebugStream.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 速さは「毎秒」で決め、1フレームぶんに直して使う（update は1フレームに1回）。
	const float kFramesPerSecond = 60.f;
	// 歩き・走りの速さ（m/フレーム）は、アニメの足運びに合わせる（合わないと足が地面を滑って見える）。
	// 値は元アニメ（In Place にする前）が1周で進む距離から測った。
	const float kWalkSpeed    = 1.2f / kFramesPerSecond; // Rifle Walk: 1.54m / 1.30秒
	const float kRunSpeed     = 4.3f / kFramesPerSecond; // Rifle Run: 2.87m / 0.67秒
	const float kAimWalkSpeed = 1.0f / kFramesPerSecond; // walking（構え）: 1.36m / 1.37秒
	const float kAimRunSpeed  = 3.1f / kFramesPerSecond; // rifle run（構え）: 2.30m / 0.73秒
	const float kStopSpeed    = 0.f;
	const float kTurnSpeed = 720.f / kFramesPerSecond; // 振り向きの速さ（度/フレーム）。真後ろへ約0.25秒
	// 加速・減速（m/フレーム²）。毎秒²の値をフレーム²に直す。
	// 止まる→走るは約0.4秒、走る→止まるは約0.27秒。減速を強めにしているのは、止まりたいときに滑り続けると操作が重く感じるため。
	const float kAcceleration = 8.f / ( kFramesPerSecond * kFramesPerSecond );
	const float kDeceleration = 12.f / ( kFramesPerSecond * kFramesPerSecond );
	// アニメを切り替えるときに前後のポーズを混ぜる長さ（フレーム）。0.2秒。
	const float kCrossFadeFrames = 0.2f * kFramesPerSecond;

	const float kHalfTurn = 180.f;
	const float kFullTurn = 360.f;
	const float kMinInputLength = 0.001f; // これより短い入力（逆方向キーの同時押しなど）は静止とみなす

	// 状態ごとのアニメ名（tools/configs/player.json の extra_animations の name）と移動の速さ。
	// 普段は銃を下げて持ち（Rifle Idle/Walk/Run）、構えている間だけ構えのアニメ（Basic Shooter Pack）にする。
	// 先頭は、アニメが見つからない状態の代わりに使う静止のアニメ。
	struct StateAnimation{
		Animator::State state;
		const char* name;
		float speed; // この状態で進む速さ（m/フレーム）
	};
	const StateAnimation kStateAnimations[] = {
		{ Animator::STATE_IDLE,     "idle",     kStopSpeed    },
		{ Animator::STATE_WALK,     "walk",     kWalkSpeed    },
		{ Animator::STATE_RUN,      "run",      kRunSpeed     },
		{ Animator::STATE_AIM_IDLE, "aim_idle", kStopSpeed    },
		{ Animator::STATE_AIM_WALK, "aim_walk", kAimWalkSpeed },
		{ Animator::STATE_AIM_RUN,  "aim_run",  kAimRunSpeed  },
	};

	// 状態で進む速さ。表に無い状態は止まる。
	float moveSpeed( Animator::State state ) {
		for ( const StateAnimation& entry : kStateAnimations ) {
			if ( entry.state == state ) {
				return entry.speed;
			}
		}
		return kStopSpeed;
	}

	// 入力から次の状態を選ぶ。構えの有無 × 静止／歩き／走り の組み合わせ。
	Animator::State chooseState( bool moving, bool running, bool aiming ) {
		if ( !moving ) {
			return aiming ? Animator::STATE_AIM_IDLE : Animator::STATE_IDLE;
		}
		if ( running ) {
			return aiming ? Animator::STATE_AIM_RUN : Animator::STATE_RUN;
		}
		return aiming ? Animator::STATE_AIM_WALK : Animator::STATE_WALK;
	}

	// ── 銃の持ち方（右手の骨の空間）──
	// 構え（rifle aiming idle の1フレーム目）で、右手が握りを、左手がハンドガードの下を支える位置を
	// Blender で求めた値。下げて持つアニメ（Rifle Idle/Walk/Run）でも両手が銃に掛かることを確かめたので、
	// 全状態で同じ持ち方のまま右手に付いていく。
	const char* kWeaponBone = "mixamorig:RightHand";
	const Vector3 kGripOnWeapon( 0.f, 1.438f, -0.169f );   // 銃のモデル上の握りの中心（m。rifle.txt の座標）
	const Vector3 kGripInHand( 0.354f, 0.38f, -9.102f );  // 握りの中心を置く、手の骨から見た位置（cm。骨格の単位）
	// 手の骨から見た銃の向き（度）。骨格と同じく Y→X→Z の順に掛ける（Matrix34 の rotateY / X / Z の順）。
	const float kGripRotationY = -156.062f;
	const float kGripRotationX = 2.115f;
	const float kGripRotationZ = 89.224f;
	// 骨格は cm（根に 0.01 倍が入っている）、銃は m なので、骨の空間では銃を100倍して等倍に戻す。
	const float kWeaponScaleInBone = 100.f;

	// 銃のモデル座標 → 手の骨の空間 の行列。握りの中心を原点へ動かし、大きさ・向きを合わせてから手の位置へ置く。
	Matrix34 weaponGrip() {
		Matrix34 grip;
		grip.setTranslation( kGripInHand );
		grip.rotateY( kGripRotationY );
		grip.rotateX( kGripRotationX );
		grip.rotateZ( kGripRotationZ );
		grip.scale( kWeaponScaleInBone );
		grip.translate( -kGripOnWeapon.x, -kGripOnWeapon.y, -kGripOnWeapon.z );
		return grip;
	}

	// 角度を [-180, 180) へ折り返す。振り向きを近い回り方向で行うため。
	float wrapSignedAngle( float degree ) {
		while ( degree >= kHalfTurn ) {
			degree -= kFullTurn;
		}
		while ( degree < -kHalfTurn ) {
			degree += kFullTurn;
		}
		return degree;
	}

	// 押されていれば 1、いなければ 0（逆方向同士を引き算して軸にする）。
	float axisValue( bool on ) {
		return on ? 1.f : 0.f;
	}

	// 足を交互に出す繰り返しのアニメか。この同士の切り替え（歩き⇔走り、構え⇔下げ）だけ足の位相を合わせる。
	// 4本の歩き・走りは、どれも1周の前半で左足・後半で右足が前に出る（元の FBX で確認済み）ので、
	// 「1周の何割まで進んだか」を引き継げば足の出方が揃う。
	bool isGaitCycle( Animator::State state ) {
		return state == Animator::STATE_WALK || state == Animator::STATE_RUN
			|| state == Animator::STATE_AIM_WALK || state == Animator::STATE_AIM_RUN;
	}
}

PlayerCharacter::PlayerCharacter( const SkinnedMeshData* data, const StaticMeshData* weapon,
const Vector3& position, float angleY ) :
	mMesh( std::make_unique< SkinnedMeshInstance >( data ) ),
	mPosition( position ),
	mAngleY( wrapSignedAngle( angleY ) ),
	mMoveDirection( 0.f, 0.f, 0.f ),
	mSpeed( 0.f ) {
	// 状態にアニメを登録する。見つからないアニメは静止のアニメで代用する
	// （レスト姿勢＝Tポーズのまま滑るより目立たない。変換し忘れはデバッグ出力で知らせる）。
	Scene::Animation idle = data->animation( kStateAnimations[ 0 ].name );
	for ( const StateAnimation& entry : kStateAnimations ) {
		Scene::Animation animation = data->animation( entry.name );
		if ( !animation ) {
			GameLib::cout << "PlayerCharacter: animation not found: " << entry.name << GameLib::endl;
			animation = idle;
		}
		mAnimator.setAnimation( entry.state, animation );
	}
	// Animator は静止状態から始まる（request は同じ状態への遷移を無視する）ので、最初のアニメはここで付ける。
	mMesh->setAnimation( mAnimator.animation(), 0.f );

	if ( weapon ) {
		int hand = data->findBone( kWeaponBone );
		if ( hand < 0 ) {
			// 骨名違いでも落ちずに素手になるだけなので、ここで知らせる。
			GameLib::cout << "PlayerCharacter: weapon bone not found: " << kWeaponBone << GameLib::endl;
		} else {
			mWeapon = std::make_unique< HeldItem >( weapon, hand, weaponGrip() );
		}
	}
}

PlayerCharacter::~PlayerCharacter() {
}

void PlayerCharacter::update( float cameraYaw ) {
	Vector3 direction;
	bool moving = readMoveDirection( cameraYaw, &direction );
	bool running = moving && InputRun();
	if ( moving ) {
		turnToward( direction );
		// 向きが追いつく前から入力方向へ進める（振り向き待ちで操作が重く感じないように）。
		mMoveDirection = direction;
	}
	Animator::State next = chooseState( moving, running, InputAim() );
	// 入力を離しても最後に進んでいた向きへ減速しながら進み、止まる。
	approachSpeed( moveSpeed( next ) );
	mPosition.madd( mMoveDirection, mSpeed );
	changeState( next );

	mMesh->setPosition( mPosition );
	mMesh->setAngleY( mAngleY );
	mMesh->update();
}

void PlayerCharacter::draw() const {
	mMesh->draw();
	if ( mWeapon ) {
		mWeapon->draw( *mMesh );
	}
}

const Vector3& PlayerCharacter::position() const {
	return mPosition;
}

bool PlayerCharacter::readMoveDirection( float cameraYaw, Vector3* direction ) const {
	float strafe  = axisValue( InputRight() ) - axisValue( InputLeft() ); // 右が正
	float forward = axisValue( InputUp() ) - axisValue( InputDown() );    // 前が正
	// カメラの水平の前方と右（ThirdPersonCamera と同じ約束: 前方はヨー0で +Z、右は 前方×上）。
	Vector3 cameraForward( Math::sin( cameraYaw ), 0.f, Math::cos( cameraYaw ) );
	Vector3 cameraRight( -Math::cos( cameraYaw ), 0.f, Math::sin( cameraYaw ) );
	direction->set( 0.f, 0.f, 0.f );
	direction->madd( cameraForward, forward );
	direction->madd( cameraRight, strafe );
	if ( direction->length() < kMinInputLength ) {
		return false;
	}
	direction->normalize(); // 斜め入力で速くならないように
	return true;
}

void PlayerCharacter::turnToward( const Vector3& direction ) {
	// rotateY(θ) は +Z を (sinθ, 0, cosθ) へ回すので、向けたい角度は atan2(x, z)。
	float targetAngle = Math::atan2( direction.x, direction.z );
	float difference = wrapSignedAngle( targetAngle - mAngleY );
	float step = Math::max( -kTurnSpeed, Math::min( kTurnSpeed, difference ) );
	mAngleY = wrapSignedAngle( mAngleY + step );
}

void PlayerCharacter::approachSpeed( float targetSpeed ) {
	if ( mSpeed < targetSpeed ) {
		mSpeed = Math::min( targetSpeed, mSpeed + kAcceleration );
	} else {
		mSpeed = Math::max( targetSpeed, mSpeed - kDeceleration );
	}
}

void PlayerCharacter::changeState( Animator::State next ) {
	Animator::State previous = mAnimator.state();
	if ( !mAnimator.request( next ) ) {
		return;
	}
	Scene::Animation animation = mAnimator.animation();
	// 歩き⇔走りは、前のアニメが1周の何割まで進んでいたかを引き継いで、同じ足の位置から始める。
	// それ以外（止まる⇔動く）は頭から。止まっている姿勢には「どちらの足が前か」が無いため。
	float startTime = 0.f;
	if ( isGaitCycle( previous ) && isGaitCycle( next ) ) {
		startTime = mMesh->animationPhase() * animation.period();
	}
	mMesh->crossFadeTo( animation, startTime, kCrossFadeFrames );
}
