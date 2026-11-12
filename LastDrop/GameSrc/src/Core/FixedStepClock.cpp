#include "GameSrc/Core/FixedStepClock.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	const int kFirstTicks = 1; // reset 直後の描画で進めるティック数
	const int kStepTicks = 1;  // コマ送り 1 回で進めるティック数
	const float kNormalTimeScale = 1.f;
}

FixedStepClock::FixedStepClock() :
	mAccumulated( 0.0 ),
	mStarted( false ),
	mPaused( false ),
	mStepRequested( false ),
	mTimeScale( kNormalTimeScale ) {
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
	if ( mPaused ) {
		// 止めている間の時間は貯めない（戻した瞬間に、止めていた分を早送りしないように）
		int stepTicks = mStepRequested ? kStepTicks : 0;
		mStepRequested = false;
		return stepTicks;
	}
	mAccumulated += elapsed.count() * mTimeScale;

	int ticks = static_cast< int >( mAccumulated / GameTime::kTickSeconds );
	if ( ticks > GameTime::kMaxTicksPerFrame ) {
		// 追いつけない分は捨てる（端数も残さない。捨てた直後に持ち越しで余分に進まないように）
		mAccumulated = 0.0;
		return GameTime::kMaxTicksPerFrame;
	}
	mAccumulated -= ticks * static_cast< double >( GameTime::kTickSeconds );
	return ticks;
}

void FixedStepClock::setPaused( bool paused ) {
	mPaused = paused;
	mStepRequested = false;
	mAccumulated = 0.0; // 止める前・戻す前の端数を持ち越さない
}

bool FixedStepClock::isPaused() const {
	return mPaused;
}

void FixedStepClock::requestStep() {
	if ( mPaused ) {
		mStepRequested = true;
	}
}

void FixedStepClock::setTimeScale( float scale ) {
	mTimeScale = scale;
}

float FixedStepClock::timeScale() const {
	return mTimeScale;
}
