#ifndef INCLUDED_FIXED_STEP_CLOCK_H
#define INCLUDED_FIXED_STEP_CLOCK_H

#include <chrono>

// 「今回の描画までに、ゲームを何ティック（GameTime.h）進めるか」を数える時計。
// 前回から経った実時間を貯めておき、1ティックぶん貯まるごとに1回と数える（端数は次回へ持ち越す）。
// 描画が 144Hz なら 0 回と 1 回が混ざり、30Hz なら毎回 2 回になる。どちらでもゲームは1秒に60ティック進む。
//
// 時間を測るだけで、ゲームを進めるのは呼び出し側（SceneManager）。
class FixedStepClock{
public:
	FixedStepClock();
	// 次に数え始めるときを「今」にする（シーンを切り替えた直後など、それまでの時間を持ち込まないため）。
	// 直後の ticksToRun は 1 を返す（最初の描画の前に必ず1回は進め、何も更新していない状態を描かないため）。
	void reset();
	// 前回の呼び出しから経った時間で、今回進めるティック数を返す。1回の描画につき1回だけ呼ぶ。
	// GameTime::kMaxTicksPerFrame を超える遅れは捨てる。
	int ticksToRun();

private:
	typedef std::chrono::steady_clock Clock;
	Clock::time_point mPrevious; // 前回 ticksToRun を呼んだ時刻
	double mAccumulated;         // まだティックにしていない時間（秒）
	bool mStarted;               // reset 後に一度でも数えたか
};

#endif
