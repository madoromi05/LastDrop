#include "GameSrc/Scene/Game/Hud/AmmoHudVisibility.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	const float kLingerSeconds = 2.f;    // 覗くのをやめた・リロードを終えた後、出したままにする長さ
	const float kFadeInSeconds = 0.15f;  // 出るときに濃くなる長さ
	const float kFadeOutSeconds = 0.3f;  // 消えるときに薄くなる長さ
	const float kOpaque = 1.f;
	const float kHidden = 0.f;

	// 0 から 1 まで seconds で動くときの、1 ティックの量。
	float stepPerTick( float seconds ) {
		return kOpaque / static_cast< float >( GameTime::secondsToTicks( seconds ) );
	}
}

AmmoHudVisibility::AmmoHudVisibility() :
	mLingerTicks( 0 ),
	mOpacity( kHidden ) {
}

void AmmoHudVisibility::update( bool wanted ) {
	if ( wanted ) {
		// 条件がある間は、切れたときに残す長さを数え直しておく。
		mLingerTicks = GameTime::secondsToTicks( kLingerSeconds );
	} else if ( mLingerTicks > 0 ) {
		--mLingerTicks;
	}
	bool shown = wanted || mLingerTicks > 0;
	if ( shown ) {
		mOpacity += stepPerTick( kFadeInSeconds );
		mOpacity = ( mOpacity > kOpaque ) ? kOpaque : mOpacity;
	} else {
		mOpacity -= stepPerTick( kFadeOutSeconds );
		mOpacity = ( mOpacity < kHidden ) ? kHidden : mOpacity;
	}
}

float AmmoHudVisibility::opacity() const {
	return mOpacity;
}

bool AmmoHudVisibility::isVisible() const {
	return mOpacity > kHidden;
}
