#include "GameSrc/Player/FirstPersonControl.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Core/GameTime.h"
#include "Framework.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// マウス1画素あたりの回転（度）。横に動かすと左右、縦に動かすと上下に回る。
	const float kLookDegreesPerPixel = 0.15f;
	// 縦の視野角（度）。覗いている間は狭めて少しズームする（アイアンサイトなので控えめ）。
	const float kHipFieldOfView = 60.f;
	const float kAimFieldOfView = 45.f;

	// 覗き具合に合わせた縦の視野角（度）。
	float fieldOfView( float aimWeight ) {
		return kHipFieldOfView + ( kAimFieldOfView - kHipFieldOfView ) * aimWeight;
	}

	// つかまれた瞬間に、視点をゾンビの顔へ向ける長さ（秒。SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 4 章）。
	const float kLookPullSeconds = 0.25f;
	const int kLookPullTicks = static_cast< int >( kLookPullSeconds * GameTime::kTicksPerSecond );
	const float kHalfTurn = 180.f;
	const float kFullTurn = 360.f;

	// degree を -180〜180 へ。
	float wrapSignedAngle( float degree ) {
		while ( degree > kHalfTurn ) {
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

	// 見える絵の揺れ（CameraShake のトラウマ。0〜1）。仕様は SummaryPlan/個別仕様/CameraShake_Recoil_Plan.txt の 3・4 章。
	// 射撃は 1 発ごとに足すが、射撃だけでは kFireTraumaLimit より揺らさない（連射しても揺れすぎない）。
	const float kFireTrauma = 0.12f;
	const float kFireTraumaLimit = 0.45f;
	const float kBiteTrauma = 0.8f;
	const float kGrabTrauma = 0.35f;
	const float kTraumaFull = 1.f;

	// 揺れ方・ぶれ方を遊ぶたびに変える種（起動からの時間）。部品ごとに足す数を変えて、同じ並びにならないようにする。
	int timeSeed( int salt ) {
		return static_cast< int >( Framework::instance().time() ) + salt;
	}
	const int kAimRecoilSalt = 1;
	const int kAimPunchSalt = 2;
	const int kShakeSalt = 3;
}

FirstPersonControl::FirstPersonControl( float startYaw ) :
	mCommand(),
	mAimRecoil( timeSeed( kAimRecoilSalt ) ),
	mAimPunch( timeSeed( kAimPunchSalt ) ),
	mShake( timeSeed( kShakeSalt ) ),
	mFireSuppressed( true ),
	mPullYaw( 0.f ),
	mPullPitch( 0.f ),
	mPullTicks( 0 ),
	mEyeDrop( 0.f ) {
	mCamera.rotate( startYaw, 0.f );
}

FirstPersonControl::~FirstPersonControl() {
	GameLib::Input::Manager::instance().setCursorLocked( false );
}

void FirstPersonControl::lockCursor() {
	GameLib::Input::Manager::instance().setCursorLocked( true );
}

void FirstPersonControl::unlockCursor() {
	GameLib::Input::Manager::instance().setCursorLocked( false );
}

bool FirstPersonControl::isCursorLocked() const {
	return GameLib::Input::Manager::instance().isCursorLocked();
}

void FirstPersonControl::readInput( float aimWeight ) {
	updateCursorLock();
	GameLib::Input::Manager input = GameLib::Input::Manager::instance();
	if ( input.isCursorLocked() ) {
		AxisStatus look = InputLook();
		// ズーム中は視野角の比で感度を下げる。同じだけマウスを動かしたとき、画面上で動く量を揃えるため。
		float degreesPerPixel = kLookDegreesPerPixel * fieldOfView( aimWeight ) / kHipFieldOfView;
		// マウスを右へ動かすと右を向く＝ヨーは減る（ヨーは上から見て反時計回りが正）。
		// マウスを下へ動かすと下を向く＝ピッチ（見下ろしが正）は増える。
		mCamera.rotate( -look.m_width * degreesPerPixel, look.m_height * degreesPerPixel );
	}

	// 押されている間の入力は今の状態で上書きし、押した瞬間の入力は使われるまで残す（consumePressed で消す）。
	mCommand.forward = axisValue( InputUp() ) - axisValue( InputDown() );
	mCommand.strafe = axisValue( InputRight() ) - axisValue( InputLeft() );
	mCommand.run = InputRun();
	mCommand.holsterPressed = mCommand.holsterPressed || InputHolster();
	mCommand.reloadPressed = mCommand.reloadPressed || InputReload();
	mCommand.aimHeld = InputAim();
	bool fireHeld = InputFireHeld();
	if ( !fireHeld ) {
		mFireSuppressed = false;
	}
	mCommand.fireHeld = fireHeld && input.isCursorLocked() && !mFireSuppressed;
}

void FirstPersonControl::readInputIdle() {
	updateCursorLock();
	clearInput();
}

void FirstPersonControl::clearInput() {
	// 押した瞬間の入力（holsterPressed・reloadPressed）は readInput と同じく consumePressed に任せる（ここで消すと、切り替えた描画の入力が落ちる）。
	mCommand.forward = 0.f;
	mCommand.strafe = 0.f;
	mCommand.run = false;
	mCommand.aimHeld = false;
	mCommand.fireHeld = false;
	mFireSuppressed = true;
}

void FirstPersonControl::updateCursorLock() {
	GameLib::Input::Manager input = GameLib::Input::Manager::instance();
	if ( InputFire() && !input.isCursorLocked() ) {
		// 解除中のクリックは「ゲームに戻る」操作として扱い、ロックし直す。
		input.setCursorLocked( true );
		mFireSuppressed = true;
	}
}

const PlayerCommand& FirstPersonControl::command() const {
	return mCommand;
}

void FirstPersonControl::consumePressed() {
	mCommand.holsterPressed = false;
	mCommand.reloadPressed = false;
}

void FirstPersonControl::updateViewEffects( bool fired ) {
	mRecoil.update( fired );
	mAimRecoil.update( fired );
	mAimPunch.update();
	if ( fired ) {
		mShake.addTrauma( kFireTrauma, kFireTraumaLimit );
	}
	mShake.update();
	// 狙いのずれは 3 つを足して渡す（どれも弾の向きに入る）。揺れは描く行列だけへ。
	float yawOffset = mAimRecoil.yawOffset() + mAimPunch.yawOffset();
	float pitchOffset = mRecoil.pitchOffset() + mAimRecoil.pitchOffset() + mAimPunch.pitchOffset();
	mCamera.setAimOffset( yawOffset, pitchOffset );
	mCamera.setShake( mShake.yaw(), mShake.pitch(), mShake.roll() );
}

void FirstPersonControl::flinch() {
	mAimPunch.punch();
	mShake.addTrauma( kBiteTrauma, kTraumaFull );
}

void FirstPersonControl::shakeOnGrab() {
	mShake.addTrauma( kGrabTrauma, kTraumaFull );
}

void FirstPersonControl::placeCamera( const Vector3* eye, float aimWeight ) {
	if ( eye ) {
		Vector3 dropped = *eye;
		dropped.y -= mEyeDrop;
		mCamera.setEyePosition( dropped );
	}
	mCamera.setFieldOfView( fieldOfView( aimWeight ) );
	mCamera.apply();
}

const FirstPersonCamera& FirstPersonControl::camera() const {
	return mCamera;
}

void FirstPersonControl::lookToward( const Vector3& target ) {
	Vector3 toTarget;
	toTarget.setSub( target, mCamera.eyePosition() );
	float flat = Math::sqrt( toTarget.x * toTarget.x + toTarget.z * toTarget.z );
	// ヨーは 0 で +Z を向き、左回りが正（FirstPersonCamera）。ピッチは見下ろしが正なので、下にある相手は正。
	float yaw = Math::atan2( toTarget.x, toTarget.z );
	float pitch = Math::atan2( -toTarget.y, flat );
	mPullYaw = wrapSignedAngle( yaw - mCamera.yaw() );
	mPullPitch = pitch - mCamera.pitch();
	mPullTicks = kLookPullTicks;
}

void FirstPersonControl::updateLookPull() {
	if ( mPullTicks <= 0 ) {
		return;
	}
	// 残りを残りのティック数で割って回す（等速。最後のティックで残りを全部回し切る）。
	float share = 1.f / static_cast< float >( mPullTicks );
	float yawStep = mPullYaw * share;
	float pitchStep = mPullPitch * share;
	mCamera.rotate( yawStep, pitchStep );
	mPullYaw -= yawStep;
	mPullPitch -= pitchStep;
	--mPullTicks;
}

void FirstPersonControl::setEyeDrop( float meters ) {
	mEyeDrop = meters;
}
