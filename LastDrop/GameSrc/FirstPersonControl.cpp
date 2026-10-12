#include "FirstPersonControl.h"
#include "Pad.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"

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

	// 押されていれば 1、いなければ 0（逆方向同士を引き算して軸にする）。
	float axisValue( bool on ) {
		return on ? 1.f : 0.f;
	}
}

FirstPersonControl::FirstPersonControl( float startYaw ) :
	mCommand(),
	mFireSuppressed( true ) {
	mCamera.rotate( startYaw, 0.f );
}

FirstPersonControl::~FirstPersonControl() {
	GameLib::Input::Manager::instance().setCursorLocked( false );
}

void FirstPersonControl::lockCursor() {
	GameLib::Input::Manager::instance().setCursorLocked( true );
}

bool FirstPersonControl::isCursorLocked() const {
	return GameLib::Input::Manager::instance().isCursorLocked();
}

void FirstPersonControl::readInput( float aimWeight ) {
	GameLib::Input::Manager input = GameLib::Input::Manager::instance();
	if ( InputMenu() ) {
		input.setCursorLocked( false );
	} else if ( InputFire() && !input.isCursorLocked() ) {
		// 解除中のクリックは「ゲームに戻る」操作として扱い、ロックし直す。
		input.setCursorLocked( true );
		mFireSuppressed = true;
	}

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
	mCommand.aimHeld = InputAim();
	bool fireHeld = InputFireHeld();
	if ( !fireHeld ) {
		mFireSuppressed = false;
	}
	mCommand.fireHeld = fireHeld && input.isCursorLocked() && !mFireSuppressed;
}

const PlayerCommand& FirstPersonControl::command() const {
	return mCommand;
}

void FirstPersonControl::consumePressed() {
	mCommand.holsterPressed = false;
}

void FirstPersonControl::placeCamera( const Vector3* eye, float aimWeight ) {
	if ( eye ) {
		mCamera.setEyePosition( *eye );
	}
	mCamera.setFieldOfView( fieldOfView( aimWeight ) );
	mCamera.apply();
}

const FirstPersonCamera& FirstPersonControl::camera() const {
	return mCamera;
}
