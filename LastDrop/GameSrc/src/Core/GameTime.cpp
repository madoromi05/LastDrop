#include "GameSrc/Core/GameTime.h"
#include <cmath>

int GameTime::secondsToTicks( float seconds ) {
	return static_cast< int >( std::lround( seconds * kTicksPerSecond ) );
}

float GameTime::ticksToSeconds( int ticks ) {
	return static_cast< float >( ticks ) / kTicksPerSecond;
}

float GameTime::remainPerTick( float halfLifeSeconds ) {
	const float kHalf = 0.5f;
	return std::pow( kHalf, 1.f / ( halfLifeSeconds * kTicksPerSecond ) );
}
