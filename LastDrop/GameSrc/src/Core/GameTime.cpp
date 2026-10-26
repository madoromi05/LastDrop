#include "GameSrc/Core/GameTime.h"
#include <cmath>

int GameTime::secondsToTicks( float seconds ) {
	return static_cast< int >( std::lround( seconds * kTicksPerSecond ) );
}

float GameTime::ticksToSeconds( int ticks ) {
	return static_cast< float >( ticks ) / kTicksPerSecond;
}
