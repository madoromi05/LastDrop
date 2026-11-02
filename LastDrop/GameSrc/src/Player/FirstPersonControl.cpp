#include "GameSrc/Player/FirstPersonControl.h"
#include "GameSrc/Player/LookParams.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Core/GameSettings.h"
#include "GameSrc/Math/ScalarMath.h"
#include "Framework.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 感度と視野角は LookParams（起動中に F6 で動かせる）。
	LookParams gLookParams;

	// 覗き具合に合わせた縦の視野角（度）。
	float fieldOfView( float aimWeight ) {
		return gLookParams.hipFieldOfView + ( gLookParams.aimFieldOfView - gLookParams.hipFieldOfView ) * aimWeight;
	}

	// 押されていれば 1、いなければ 0（逆方向同士を引き算して軸にする）。
	float axisValue( bool on ) {
		return on ? 1.f : 0.f;
	}

	// 移動の軸の上限（キーとスティックを同時に入れても PlayerCommand の -1〜1 に収める）。
	const float kMoveAxisLimit = 1.f;

	// スティックで回す量に使う、1 回の描画の長さの上限（秒）。一時停止や読み込みで空いた時間を、1 度に回さないため。
	const float kMaxStickLookSeconds = 0.1f;
	const float kMillisecondsPerSecond = 1000.f;

	// 前の描画からの秒数（スティックの「傾き＝速さ」を回す角度にするため）。
	float stickLookSeconds() {
		float seconds = static_cast< float >( Framework::instance().previousFrameInterval() ) / kMillisecondsPerSecond;
		return Math::min( seconds, kMaxStickLookSeconds );
	}

	// スティックの傾き（-1〜1）を、回る速さの割合（-1〜1）にする。向きは残して大きさだけ累乗する。
	float stickLookRate( float tilt ) {
		float rate = Math::pow( Math::abs( tilt ), gLookParams.stickLookExponent );
		return ( tilt < 0.f ) ? -rate : rate;
	}

	// 見える絵の揺れ（CameraShake のトラウマ。0〜1）。仕様は SummaryPlan/個別仕様/CameraShake_Recoil_Plan.txt の 3・4 章。
	// 射撃は 1 発ごとに足すが、射撃だけでは kFireTraumaLimit より揺らさない（連射しても揺れすぎない）。
	const float kFireTrauma = 0.12f;
	const float kFireTraumaLimit = 0.45f;
	const float kZombieHitTrauma = 0.8f;
	const float kTraumaFull = 1.f;

	// 揺れ方・ぶれ方を遊ぶたびに変える種（起動からの時間）。部品ごとに足す数を変えて、同じ並びにならないようにする。
	int timeSeed( int salt ) {
		return static_cast< int >( Framework::instance().time() ) + salt;
	}
	const int kAimRecoilSalt = 1;
	const int kAimPunchSalt = 2;
	const int kShakeSalt = 3;
}

LookParams& lookParams() {
	return gLookParams;
}

FirstPersonControl::FirstPersonControl( float startYaw, const GameSettings* settings ) :
	mSettings( settings ),
	mCommand(),
	mAimRecoil( timeSeed( kAimRecoilSalt ) ),
	mAimPunch( timeSeed( kAimPunchSalt ) ),
	mShake( timeSeed( kShakeSalt ) ),
	mPendingLookYaw( 0.f ),
	mPendingLookPitch( 0.f ),
	mFireSuppressed( true ),
	mEyeDrop( 0.f ) {
	ASSERT( mSettings && "FirstPersonControl : settings is null." );
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
		AxisStatus stick = InputLookStick();
		// ズーム中は視野角の比で感度を下げる。同じだけマウスを動かしたとき、画面上で動く量を揃えるため。
		// 設定画面の感度（倍率）は、その上から掛ける（覗いたときの遅くなり方の比は変わらない）。スティックにも同じ倍率を掛ける。
		float sensitivity = mSettings->lookSensitivityScale() * fieldOfView( aimWeight ) / gLookParams.hipFieldOfView;
		float degreesPerPixel = gLookParams.lookDegreesPerPixel * sensitivity;
		// スティックは傾き＝回る速さなので、この描画の秒数を掛けて角度にする（マウスは動かした量そのものが角度になる）。
		float stickSeconds = stickLookSeconds() * sensitivity;
		float stickYaw = stickLookRate( stick.m_width ) * gLookParams.stickYawDegreesPerSecond * stickSeconds;
		float stickPitch = stickLookRate( stick.m_height ) * gLookParams.stickPitchDegreesPerSecond * stickSeconds;
		// マウス（スティック）を右へ動かすと右を向く＝ヨーは減る（ヨーは上から見て反時計回りが正）。
		// マウス（スティック）を下へ動かすと下を向く＝ピッチ（見下ろしが正）は増える。
		float yawDelta = -look.m_width * degreesPerPixel - stickYaw;
		float pitchDelta = look.m_height * degreesPerPixel + stickPitch;
		mCamera.rotate( yawDelta, pitchDelta );
		mPendingLookYaw += yawDelta;
		mPendingLookPitch += pitchDelta;
	}

	// 押されている間の入力は今の状態で上書きし、押した瞬間の入力は使われるまで残す（consumePressed で消す）。
	// キーとパッドのスティックを足す。スティックの縦は下が正なので、前（上）を正にするため引く。
	AxisStatus move = InputMove();
	mCommand.forward = ScalarMath::clampSigned( axisValue( InputUp() ) - axisValue( InputDown() ) - move.m_height, kMoveAxisLimit );
	mCommand.strafe = ScalarMath::clampSigned( axisValue( InputRight() ) - axisValue( InputLeft() ) + move.m_width, kMoveAxisLimit );
	mCommand.run = InputRun();
	mCommand.holsterPressed = mCommand.holsterPressed || InputHolster();
	mCommand.reloadPressed = mCommand.reloadPressed || InputReload();
	mCommand.flashlightPressed = mCommand.flashlightPressed || InputFlashlight();
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
	// 押した瞬間の入力（holsterPressed・reloadPressed・flashlightPressed）は readInput と同じく consumePressed に任せる（ここで消すと、切り替えた描画の入力が落ちる）。
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
	mCommand.flashlightPressed = false;
}

void FirstPersonControl::updateViewEffects( bool fired, const AimSwayBody& body ) {
	mRecoil.update( fired );
	mAimRecoil.update( fired );
	mAimPunch.update();
	if ( fired ) {
		mShake.addTrauma( kFireTrauma, kFireTraumaLimit );
	}
	mShake.update();
	// ためたマウスの回転は、このティックで使い切る（1 回の描画に 2 ティックあれば、2 つ目は 0）。
	mSway.update( body, mPendingLookYaw, mPendingLookPitch );
	mPendingLookYaw = 0.f;
	mPendingLookPitch = 0.f;
	// 狙いのずれは 3 つを足して渡す（どれもカメラごと動き、弾の向きに入る）。揺れは描く行列だけへ。
	// 照準のぶれは銃と弾の向きだけへ（画面の中央は動かさない）。
	float yawOffset = mAimRecoil.yawOffset() + mAimPunch.yawOffset();
	float pitchOffset = mRecoil.pitchOffset() + mAimRecoil.pitchOffset() + mAimPunch.pitchOffset();
	mCamera.setAimOffset( yawOffset, pitchOffset );
	mCamera.setShake( mShake.yaw(), mShake.pitch(), mShake.roll() );
	mCamera.setWeaponSway( mSway.yawOffset(), mSway.pitchOffset() );
}

void FirstPersonControl::flinch() {
	mAimPunch.punch();
	mShake.addTrauma( kZombieHitTrauma, kTraumaFull );
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

void FirstPersonControl::setEyeDrop( float meters ) {
	mEyeDrop = meters;
}
