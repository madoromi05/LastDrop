#include "GameSrc/Raid/RaidClock.h"
#include "GameSrc/Core/GameTime.h"

RaidClock::RaidClock() :
	mElapsedTicks( 0 ) {
}

void RaidClock::tick() {
	++mElapsedTicks;
}

float RaidClock::elapsedSeconds() const {
	return GameTime::ticksToSeconds( mElapsedTicks );
}
