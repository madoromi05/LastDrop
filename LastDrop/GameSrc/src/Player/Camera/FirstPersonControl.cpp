#include "GameSrc/Player/Camera/FirstPersonControl.h"
#include "GameSrc/Player/Camera/LookParams.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Core/GameSettings.h"
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
}

LookParams& lookParams() {
	return gLookParams;
}

FirstPersonControl::FirstPersonControl( float startYaw, const GameSettings* settings ) :
	mSettings( settings ),
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

bool FirstPersonControl::isFireSuppressed() const {
	return mCommands.isFireSuppressed();
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
		mEffects.addLook( yawDelta, pitchDelta );
	}

	// 押されている間の入力は今の状態で上書きし、押した瞬間の入力は使われるまで残す（consumePressed で消す）。
	mCommands.read( input.isCursorLocked() );
}

void FirstPersonControl::readInputIdle() {
	updateCursorLock();
	clearInput();
}

void FirstPersonControl::clearInput() {
	mCommands.clear();
}

void FirstPersonControl::updateCursorLock() {
	GameLib::Input::Manager input = GameLib::Input::Manager::instance();
	if ( InputFire() && !input.isCursorLocked() ) {
		// 解除中のクリックは「ゲームに戻る」操作として扱い、ロックし直す。
		input.setCursorLocked( true );
		mCommands.suppressFire();
	}
}

const PlayerCommand& FirstPersonControl::command() const {
	return mCommands.command();
}

void FirstPersonControl::consumePressed() {
	mCommands.consumePressed();
}

void FirstPersonControl::updateViewEffects( float shotRecoil, const AimSwayBody& body ) {
	mEffects.update( shotRecoil, body, &mCamera );
}

void FirstPersonControl::flinch() {
	mEffects.flinch();
}

void FirstPersonControl::shakeOnGrab() {
	mEffects.shakeOnGrab();
}

void FirstPersonControl::shakeOnHeavyHit( bool headBurst ) {
	mEffects.shakeOnHeavyHit( headBurst );
}

void FirstPersonControl::lookToward( const Vector3& target ) {
	mLookPull.start( mCamera, target );
}

void FirstPersonControl::updateLookPull() {
	mLookPull.update( &mCamera );
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
