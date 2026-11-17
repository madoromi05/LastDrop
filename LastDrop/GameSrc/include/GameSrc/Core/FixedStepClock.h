#ifndef INCLUDED_FIXED_STEP_CLOCK_H
#define INCLUDED_FIXED_STEP_CLOCK_H

#include <chrono>

// 「今回の描画までに、ゲームを何ティック（GameTime.h）進めるか」を数える時計
// 前回から経った実時間を貯めておき、1ティックぶん貯まるごとに1回と数える（端数は次回へ持ち越す）
// 描画が 144Hz なら 0 回と 1 回が混ざり、30Hz なら毎回 2 回になる。どちらでもゲームは1秒に60ティック進む
//
// 時間を測るだけで、ゲームを進めるのは呼び出し側（SceneManager）
class FixedStepClock{
public:
	FixedStepClock();
	// 次に数え始めるときを「今」にする（シーンを切り替えた直後など、それまでの時間を持ち込まないため）
	// 直後の ticksToRun は 1 を返す（最初の描画の前に必ず1回は進め、何も更新していない状態を描かないため）
	void reset();
	// 前回の呼び出しから経った時間で、今回進めるティック数を返す。1回の描画につき1回だけ呼ぶ
	// GameTime::kMaxTicksPerFrame を超える遅れは捨てる
	int ticksToRun();

	// ── 開発用のエディタ（Scene の欄）から使う。ESC の一時停止（GameScene。メニューを出し音も止める）とは別 ──
	// 止める。止めている間は 0 ティック（絵は描き続ける）。止めている間の時間は貯めない（戻した瞬間に早送りにならない）
	void setPaused( bool paused );
	bool isPaused() const;
	void requestStep();	// 止めている間に、次の描画で 1 ティックだけ進める（コマ送り）。止めていなければ何もしない
	// 時間の倍率（1 で通常。0.25 なら 4 倍のスロー）。ティックの長さは変えず、進めるティックの数で調整する
	void setTimeScale( float scale );
	float timeScale() const;

private:
	using Clock = std::chrono::steady_clock;
	Clock::time_point mPrevious; // 前回 ticksToRun を呼んだ時刻
	double mAccumulated;         // まだティックにしていない時間（秒）
	bool mStarted;               // reset 後に一度でも数えたか
	bool mPaused;
	bool mStepRequested;
	float mTimeScale;
};

#endif
