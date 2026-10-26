#include "GameSrc/Weapon/FireControl.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	// 連射の速さ（発/分）。AKM の実銃の値（約600発/分）に合わせる。
	const float kRoundsPerMinute = 600.f;
	const float kSecondsPerMinute = 60.f;
	// 1発ごとの間隔（ティック）。600発/分なら 6 ティックに1発。
	const int kFireIntervalTicks = GameTime::secondsToTicks( kSecondsPerMinute / kRoundsPerMinute );
}

FireControl::FireControl() :
	mCooldownTicks( 0 ) {
}

bool FireControl::update( bool triggerHeld ) {
	if ( mCooldownTicks > 0 ) {
		--mCooldownTicks;
	}
	if ( !triggerHeld || mCooldownTicks > 0 ) {
		return false;
	}
	mCooldownTicks = kFireIntervalTicks;
	return true;
}
