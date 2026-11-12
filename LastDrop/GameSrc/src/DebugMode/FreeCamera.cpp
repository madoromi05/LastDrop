#include "GameSrc/DebugMode/FreeCamera.h"
#include "GameSrc/DebugMode/DebugSwitches.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 真上・真下ちょうどだと、ビュー行列を作る上向き（+Y）と視線が重なって向きが決まらないので、少し手前で止める
	const float kPitchLimit = 89.f;
	// マウス1画素あたりの回転（度）。自キャラの視点（FirstPersonControl）と同じ感度にして、切り替えても手の感覚を変えない
	const float kLookDegreesPerPixel = 0.15f;
	// 動く速さ（m/ティック）。町の1区画（数十 m）を1〜2秒で渡れる速さ。人の走る速さの数倍
	const float kMoveSpeed = 20.f / GameTime::kTicksPerSecond;
	// Shift を押している間の倍率。町の端から端まで（数百 m）を数秒で渡るため
	const float kFastMultiplier = 4.f;
	const Vector3 kUp( 0.f, 1.f, 0.f );

	// 押されていれば 1、いなければ 0（逆方向同士を引き算して軸にする）
	float axisValue( bool on ) {
		return on ? 1.f : 0.f;
	}
}

FreeCamera::FreeCamera() :
	mCamera( kPitchLimit ),
	mActive( false ) {
}

void FreeCamera::handleToggle( const FirstPersonCamera& start ) {
	bool& requested = debugSwitches().toggleFreeCamera;
	if ( !requested ) {
		return;
	}
	requested = false; // 読んだので下ろす
	mActive = !mActive;
	if ( mActive ) {
		// 前に使ったときの位置からではなく、毎回自キャラの目から始める（どこにいたか見失わないように）
		mCamera = FirstPersonCamera( kPitchLimit );
		mCamera.rotate( start.yaw(), start.pitch() );
		mCamera.setEyePosition( start.eyePosition() );
	}
}

bool FreeCamera::isActive() const {
	return mActive;
}

void FreeCamera::look() {
	if ( !mActive || !GameLib::Input::Manager::instance().isCursorLocked() ) {
		return;
	}
	AxisStatus look = InputLook();
	// 向きの正負は FirstPersonControl::readInput と同じ（右へ動かすと右を向き、下へ動かすと下を向く）
	mCamera.rotate( -look.m_width * kLookDegreesPerPixel, look.m_height * kLookDegreesPerPixel );
}

void FreeCamera::move() {
	if ( !mActive ) {
		return;
	}
	// 前後は視線の向きそのまま（見下ろして W なら降りていく）、左右は水平、上下は世界の真上・真下
	// 右の向きは PlayerCharacter の cameraRight と同じ式
	float yaw = mCamera.yaw();
	Vector3 right( -Math::cos( yaw ), 0.f, Math::sin( yaw ) );
	Vector3 direction( 0.f, 0.f, 0.f );
	direction.madd( mCamera.viewDirection(), axisValue( InputUp() ) - axisValue( InputDown() ) );
	direction.madd( right, axisValue( InputRight() ) - axisValue( InputLeft() ) );
	direction.madd( kUp, axisValue( InputDebugCameraUp() ) - axisValue( InputDebugCameraDown() ) );
	if ( direction.squareLength() == 0.f ) {
		return;
	}
	// 斜めに押しても速くならないよう、向きだけにしてから速さを掛ける
	direction.normalize();
	float speed = InputRun() ? kMoveSpeed * kFastMultiplier : kMoveSpeed;
	Vector3 eye;
	eye.setMadd( mCamera.eyePosition(), direction, speed );
	mCamera.setEyePosition( eye );
}

void FreeCamera::apply() const {
	mCamera.apply();
}

const FirstPersonCamera& FreeCamera::camera() const {
	return mCamera;
}
