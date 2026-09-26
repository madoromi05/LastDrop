#include "PlayerCharacter.h"
#include "SkinnedMeshData.h"
#include "SkinnedMeshInstance.h"
#include "HeldItem.h"
#include "FirstPersonBody.h"
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
	const float kWalkSpeed      = 1.4f / kFramesPerSecond; // Walking（素手）
	const float kRunSpeed       = 3.2f / kFramesPerSecond; // Slow Run（素手）
	const float kRifleWalkSpeed = 1.0f / kFramesPerSecond; // walking（Shooter Pack）: 1.36m / 1.37秒
	const float kRifleRunSpeed  = 3.1f / kFramesPerSecond; // rifle run（Shooter Pack）: 2.30m / 0.73秒
	const float kStopSpeed      = 0.f;
	// 加速・減速（m/フレーム²）。毎秒²の値をフレーム²に直す。
	// 止まる→走るは約0.4秒、走る→止まるは約0.27秒。減速を強めにしているのは、止まりたいときに滑り続けると操作が重く感じるため。
	const float kAcceleration = 8.f / ( kFramesPerSecond * kFramesPerSecond );
	const float kDeceleration = 12.f / ( kFramesPerSecond * kFramesPerSecond );
	// アニメを切り替えるときに前後のポーズを混ぜる長さ（フレーム）。0.2秒。
	const float kCrossFadeFrames = 0.2f * kFramesPerSecond;
	const float kNoWeight   = 0.f; // 補正の効き具合（両手持ち）
	const float kFullWeight = 1.f;

	const float kHalfTurn = 180.f;
	const float kFullTurn = 360.f;
	const float kMinInputLength = 0.001f; // これより短い入力（逆方向キーの同時押しなど）は静止とみなす

	// 状態ごとのアニメ名（tools/configs/player.json の extra_animations の name）と移動の速さ。
	// 武器をしまっている間は素手のアニメ、持っている間は銃を肩の高さに持つアニメ（Basic Shooter Pack）。
	// 覗く（ADS）は専用のアニメを持たず、持っているアニメのまま FirstPersonBody が腕と銃を目の前へ寄せる。
	// 先頭は、アニメが見つからない状態の代わりに使う静止のアニメ。
	struct StateAnimation{
		Animator::State state;
		const char* name;
		float speed; // この状態で進む速さ（m/フレーム）
	};
	const StateAnimation kStateAnimations[] = {
		{ Animator::STATE_IDLE,       "idle",       kStopSpeed      },
		{ Animator::STATE_WALK,       "walk",       kWalkSpeed      },
		{ Animator::STATE_RUN,        "run",        kRunSpeed       },
		{ Animator::STATE_RIFLE_IDLE, "rifle_idle", kStopSpeed      },
		{ Animator::STATE_RIFLE_WALK, "rifle_walk", kRifleWalkSpeed },
		{ Animator::STATE_RIFLE_RUN,  "rifle_run",  kRifleRunSpeed  },
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

	// 入力から次の状態を選ぶ。武器を持っているか × 静止／歩き／走り の組み合わせ。
	Animator::State chooseState( bool moving, bool running, bool drawn ) {
		if ( !moving ) {
			return drawn ? Animator::STATE_RIFLE_IDLE : Animator::STATE_IDLE;
		}
		if ( running ) {
			return drawn ? Animator::STATE_RIFLE_RUN : Animator::STATE_RUN;
		}
		return drawn ? Animator::STATE_RIFLE_WALK : Animator::STATE_WALK;
	}

	// ── 銃の持ち方（右手の骨の空間）──
	// 構え（rifle aiming idle の1フレーム目）で、右手が握りを、左手がハンドガードの下を支える位置を
	// Blender で求めた値。Shooter Pack の歩き・走りでも同じ持ち方のまま右手に付いていく。
	const char* kWeaponBone = "mixamorig:RightHand";
	const Vector3 kGripOnWeapon( 0.f, 1.438f, -0.169f );   // 銃のモデル上の握りの中心（m。rifle.txt の座標）
	const Vector3 kGripInHand( 0.354f, 0.38f, -9.102f );  // 握りの中心を置く、手の骨から見た位置（cm。骨格の単位）
	// 手の骨から見た銃の向き（度）。骨格と同じく Y→X→Z の順に掛ける（Matrix34 の rotateY / X / Z の順）。
	const float kGripRotationY = -156.062f;
	const float kGripRotationX = 2.115f;
	const float kGripRotationZ = 89.224f;
	// 骨格は cm（根に 0.01 倍が入っている）、銃は m なので、骨の空間では銃を100倍して等倍に戻す。
	const float kWeaponScaleInBone = 100.f;
	// 覗いたときに目を置く点（銃のモデル座標。m）。AKM のサイト線（リアサイト z≈0.05 → フロントサイト z≈0.43、
	// どちらも上端 y≈1.597）を、頂点の上端の並びから求め、ほお付けの位置まで後ろへ延ばした所。
	// 目の前のリアサイトが大きく見えすぎる・機関部が視界を塞ぐなら z を、サイトが上下にずれるなら y を調整する。
	const Vector3 kSightEyeOnWeapon( 0.f, 1.600f, -0.12f );
	// 銃を支える手。どのアニメでも、この手の骨が銃の上の決まった点に来るよう右手首を曲げる（FirstPersonBody）。
	const char* kSupportHandBone = "mixamorig:LeftHand";
	// 持ち方（kGripInHand など）を合わせたアニメと時刻。このポーズで左手が銃のどこにあるかを「左手の来るべき点」にする。
	// 数値で持たずにアニメから求めるのは、持ち方を合わせ直したときに一緒に合わせ直す値を増やさないため。
	const char* kGripCalibrationAnimation = "rifle_idle"; // rifle aiming idle
	const float kGripCalibrationTime = 0.f;               // 1フレーム目

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

	// 持ち方を合わせたアニメで、支える手の骨が銃のどこにあるか（銃のモデル座標）を out へ書く。
	// そのアニメが無ければ false（両手持ちの補正をしない）。起動時に1回だけ呼ぶ（骨格を1つ作って捨てる）。
	bool findSupportHandOnWeapon( const SkinnedMeshData* data, int hand, int supportHand, const Matrix34& grip, Vector3* out ) {
		Scene::Animation calibration = data->animation( kGripCalibrationAnimation );
		if ( !calibration ) {
			GameLib::cout << "PlayerCharacter: animation not found: " << kGripCalibrationAnimation << GameLib::endl;
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

	// 角度を [-180, 180) へ折り返す。
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

	// 足を交互に出す繰り返しのアニメか。この同士の切り替え（歩き⇔走り、素手⇔銃）だけ足の位相を合わせる。
	// 4本の歩き・走りは、どれも1周の前半で左足・後半で右足が前に出る（元の FBX で確認済み）ので、
	// 「1周の何割まで進んだか」を引き継げば足の出方が揃う。
	bool isGaitCycle( Animator::State state ) {
		return state == Animator::STATE_WALK || state == Animator::STATE_RUN
			|| state == Animator::STATE_RIFLE_WALK || state == Animator::STATE_RIFLE_RUN;
	}
}

PlayerCharacter::PlayerCharacter( const SkinnedMeshData* data, const StaticMeshData* weapon,
const Vector3& position, float angleY ) :
	mBody( std::make_unique< FirstPersonBody >( data ) ),
	mMesh( std::make_unique< SkinnedMeshInstance >( data ) ),
	mPosition( position ),
	mAngleY( wrapSignedAngle( angleY ) ),
	mMoveDirection( 0.f, 0.f, 0.f ),
	mSpeed( 0.f ),
	mHoldWeight( 0.f ) {
	mMesh->setPoseModifier( mBody.get() );

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
	// 武器を持って始まるなら、最初から持った静止のアニメにする（1フレーム目から素手→銃のフェードを見せない）。
	mAnimator.request( chooseState( false, false, mStance.isDrawn() ) );
	mMesh->setAnimation( mAnimator.animation(), 0.f );

	if ( weapon ) {
		int hand = data->findBone( kWeaponBone );
		if ( hand < 0 ) {
			// 骨名違いでも落ちずに素手になるだけなので、ここで知らせる。
			GameLib::cout << "PlayerCharacter: weapon bone not found: " << kWeaponBone << GameLib::endl;
		} else {
			Matrix34 grip = weaponGrip();
			mWeapon = std::make_unique< HeldItem >( weapon, hand, grip );
			// 支える手が見つからなければ、右手だけで持つ（左手が銃から離れることがあるだけで落ちない）。
			int supportHand = data->findBone( kSupportHandBone );
			Vector3 supportOnWeapon( 0.f, 0.f, 0.f );
			if ( supportHand < 0 ) {
				GameLib::cout << "PlayerCharacter: support hand bone not found: " << kSupportHandBone << GameLib::endl;
			} else if ( !findSupportHandOnWeapon( data, hand, supportHand, grip, &supportOnWeapon ) ) {
				supportHand = -1;
			}
			mBody->setWeapon( hand, grip, kSightEyeOnWeapon, supportHand, supportOnWeapon );
		}
	}
	mHoldWeight = mStance.isDrawn() ? kFullWeight : kNoWeight;
}

PlayerCharacter::~PlayerCharacter() {
}

void PlayerCharacter::update( float cameraYaw, float cameraPitch ) {
	// 一人称では体の向き＝視線の向き。振り向きを遅らせると、視点と腕・銃の向きがずれて見えるため即座に合わせる。
	mAngleY = wrapSignedAngle( cameraYaw );
	mBody->setPitch( cameraPitch );

	Vector3 direction;
	bool moving = readMoveDirection( cameraYaw, &direction );
	bool running = moving && InputRun();
	if ( moving ) {
		mMoveDirection = direction;
	}
	mStance.update( InputHolster(), InputAim(), running );
	approachHoldWeight();
	mBody->setHoldWeight( mHoldWeight );
	mBody->setAimWeight( mStance.aimWeight() );
	Animator::State next = chooseState( moving, running, mStance.isDrawn() );
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
	// しまった武器は描かない（背中に背負った見た目は、一人称では見えないので持たない）。
	// 出し入れのアニメが無いので、切り替えた瞬間に手へ現れる・消える。
	if ( mWeapon && mStance.isDrawn() ) {
		mWeapon->draw( *mMesh );
	}
}

const Vector3& PlayerCharacter::position() const {
	return mPosition;
}

Vector3 PlayerCharacter::eyePosition() const {
	// FirstPersonBody が骨格の根の空間で求めた目を、置き場所・向きでワールドへ置く（頂点と同じ式）。
	Vector3 eye;
	mMesh->rootMatrix().mul( &eye, mBody->eyePosition() );
	return eye;
}

float PlayerCharacter::aimWeight() const {
	return mStance.aimWeight();
}

bool PlayerCharacter::readMoveDirection( float cameraYaw, Vector3* direction ) const {
	float strafe  = axisValue( InputRight() ) - axisValue( InputLeft() ); // 右が正
	float forward = axisValue( InputUp() ) - axisValue( InputDown() );    // 前が正
	// カメラの水平の前方と右（FirstPersonCamera と同じ約束: 前方はヨー0で +Z、右は 前方×上）。
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

void PlayerCharacter::approachSpeed( float targetSpeed ) {
	if ( mSpeed < targetSpeed ) {
		mSpeed = Math::min( targetSpeed, mSpeed + kAcceleration );
	} else {
		mSpeed = Math::max( targetSpeed, mSpeed - kDeceleration );
	}
}

void PlayerCharacter::approachHoldWeight() {
	// 素手⇔銃のアニメのクロスフェードと同じ長さで効かせる／外す。素手のポーズに右手首の曲げが急に入ると手首が跳ねて見えるため。
	float target = mStance.isDrawn() ? kFullWeight : kNoWeight;
	float step = kFullWeight / kCrossFadeFrames;
	if ( mHoldWeight < target ) {
		mHoldWeight = Math::min( target, mHoldWeight + step );
	} else {
		mHoldWeight = Math::max( target, mHoldWeight - step );
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
