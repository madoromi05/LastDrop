#include "GameSrc/Player/PlayerGrabComponent.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	// ゾンビに離されてから、またつかまれるようになるまで（秒）
	const float kGrabCooldownSeconds = 1.f;
	const int kGrabCooldownTicks = GameTime::secondsToTicks( kGrabCooldownSeconds );
}

PlayerGrabComponent::PlayerGrabComponent() :
	mHeld( false ),
	mGrabCooldownTicks( 0 ) {
}

void PlayerGrabComponent::setHeld( bool held ) {
	if ( mHeld && !held ) {
		mGrabCooldownTicks = kGrabCooldownTicks; // 離された
	} else if ( !held && mGrabCooldownTicks > 0 ) {
		--mGrabCooldownTicks;
	}
	mHeld = held;
}

bool PlayerGrabComponent::isHeld() const {
	return mHeld;
}

bool PlayerGrabComponent::canBeGrabbed() const {
	return !mHeld && mGrabCooldownTicks <= 0;
}
