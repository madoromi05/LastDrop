#include "GameSrc/Core/FixedStepClock.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	const int kFirstTicks = 1; // reset 直後の描画で進めるティック数
}

FixedStepClock::FixedStepClock() :
	mAccumulated( 0.0 ),
	mStarted( false ) {
}

void FixedStepClock::reset() {
	mAccumulated = 0.0;
	mStarted = false;
}

int FixedStepClock::ticksToRun() {
	Clock::time_point now = Clock::now();
	if ( !mStarted ) {
		mPrevious = now;
		mStarted = true;
		return kFirstTicks;
	}
	std::chrono::duration< double > elapsed = now - mPrevious;
	mPrevious = now;
	mAccumulated += elapsed.count();

	int ticks = static_cast< int >( mAccumulated / GameTime::kTickSeconds );
	if ( ticks > GameTime::kMaxTicksPerFrame ) {
		// 追いつけない分は捨てる（端数も残さない。捨てた直後に持ち越しで余分に進まないように）。
		mAccumulated = 0.0;
		return GameTime::kMaxTicksPerFrame;
	}
	mAccumulated -= ticks * static_cast< double >( GameTime::kTickSeconds );
	return ticks;
}
