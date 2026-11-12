#include "GameSrc/Player/Camera/LookPull.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// つかまれた瞬間に、視点をゾンビの顔へ向ける長さ（秒）
	const float kLookPullSeconds = 0.25f;
	const int kLookPullTicks = GameTime::secondsToTicks( kLookPullSeconds );
}

LookPull::LookPull() :
	mYaw( 0.f ),
	mPitch( 0.f ),
	mTicks( 0 ) {
}

void LookPull::start( const FirstPersonCamera& camera, const Vector3& target ) {
	Vector3 toTarget;
	toTarget.setSub( target, camera.eyePosition() );
	float flat = Math::sqrt( toTarget.x * toTarget.x + toTarget.z * toTarget.z );
	// ヨーは 0 で +Z を向き、左回りが正（FirstPersonCamera）。ピッチは見下ろしが正なので、下にある相手は正
	float yaw = AngleMath::headingOf( toTarget );
	float pitch = Math::atan2( -toTarget.y, flat );
	mYaw = AngleMath::difference( yaw, camera.yaw() );
	mPitch = pitch - camera.pitch();
	mTicks = kLookPullTicks;
}

void LookPull::update( FirstPersonCamera* camera ) {
	ASSERT( camera && "LookPull::update : camera is null." );
	if ( mTicks <= 0 ) {
		return;
	}
	// 残りを残りのティック数で割って回す（等速。最後のティックで残りを全部回し切る）
	float share = 1.f / static_cast< float >( mTicks );
	float yawStep = mYaw * share;
	float pitchStep = mPitch * share;
	camera->rotate( yawStep, pitchStep );
	mYaw -= yawStep;
	mPitch -= pitchStep;
	--mTicks;
}
