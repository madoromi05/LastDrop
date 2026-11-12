#include "GameSrc/Weapon/FireControl.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"

namespace{
	const float kSecondsPerMinute = 60.f;
}

FireControl::FireControl( float roundsPerMinute, bool fullAuto ) :
	mIntervalTicks( 0 ),
	mFullAuto( fullAuto ),
	mCooldownTicks( 0 ),
	mWasHeld( false ) {
	ASSERT( roundsPerMinute > 0.f && "FireControl : roundsPerMinute must be positive." );
	// 600発/分なら 6 ティックに1発
	mIntervalTicks = GameTime::secondsToTicks( kSecondsPerMinute / roundsPerMinute );
}

bool FireControl::update( bool triggerHeld ) {
	if ( mCooldownTicks > 0 ) {
		--mCooldownTicks;
	}
	// セミオートは、引いた瞬間（前のティックは離していた）だけ撃つ。間隔の途中で引いた分は撃たない（引き直してもらう）
	bool wantsShot = triggerHeld && ( mFullAuto || !mWasHeld );
	mWasHeld = triggerHeld;
	if ( !wantsShot || mCooldownTicks > 0 ) {
		return false;
	}
	mCooldownTicks = mIntervalTicks;
	return true;
}
