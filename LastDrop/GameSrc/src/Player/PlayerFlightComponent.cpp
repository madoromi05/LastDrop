#include "GameSrc/Player/PlayerFlightComponent.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/AngleMath.h"

using namespace GameLib::Math;

namespace{
	const float kMoveSpeed = 20.f / GameTime::kTicksPerSecond;	// 飛ぶ速さ（m/ティック）。町の1区画（数十 m）を1〜2秒で渡れる速さ。人の走る速さの数倍
	const float kFastMultiplier = 4.f;							// Shift を押している間の倍率。町の端から端まで（数百 m）を数秒で渡るため
	const Vector3 kUp( 0.f, 1.f, 0.f );
}

PlayerFlightComponent::PlayerFlightComponent() :
	mFlying( false ) {
}

void PlayerFlightComponent::toggle() {
	mFlying = !mFlying;
}

bool PlayerFlightComponent::isFlying() const {
	return mFlying;
}

Vector3 PlayerFlightComponent::step( const PlayerCommand& command, float cameraYaw, float cameraPitch ) const {
	// 前後は視線の向きそのまま、左右は水平、上下は世界の真上・真下
	Vector3 direction( 0.f, 0.f, 0.f );
	direction.madd( AngleMath::directionOf( cameraYaw, cameraPitch ), command.forward );
	direction.madd( AngleMath::rightOf( cameraYaw ), command.strafe );
	direction.madd( kUp, command.rise );
	if ( direction.squareLength() == 0.f ) {
		return direction;
	}
	direction.normalize();	// 斜めに押しても速くならないよう、向きだけにしてから速さを掛ける
	direction *= command.run ? kMoveSpeed * kFastMultiplier : kMoveSpeed;
	return direction;
}
