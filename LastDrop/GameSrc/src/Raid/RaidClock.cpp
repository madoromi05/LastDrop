#include "GameSrc/Raid/RaidClock.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"

RaidClock::RaidClock( float seconds ) :
	mTotalTicks( GameTime::secondsToTicks( seconds ) ),
	mRemainingTicks( mTotalTicks ) {
	ASSERT( mTotalTicks > 0 && "RaidClock : seconds must be positive." );
}

void RaidClock::tick() {
	if ( mRemainingTicks > 0 ) {
		--mRemainingTicks;
	}
}

float RaidClock::remainingSeconds() const {
	return GameTime::ticksToSeconds( mRemainingTicks );
}

float RaidClock::elapsedSeconds() const {
	// F6 で残りを始めより長くしたときも、負にしない。
	int elapsed = mTotalTicks - mRemainingTicks;
	return GameTime::ticksToSeconds( elapsed > 0 ? elapsed : 0 );
}

bool RaidClock::isExpired() const {
	return mRemainingTicks <= 0;
}

void RaidClock::setRemainingSeconds( float seconds ) {
	ASSERT( seconds >= 0.f && "RaidClock::setRemainingSeconds : negative." );
	mRemainingTicks = GameTime::secondsToTicks( seconds );
}
