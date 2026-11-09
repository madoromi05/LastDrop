#ifndef INCLUDED_RAID_CLOCK_H
#define INCLUDED_RAID_CLOCK_H

// 1 回の出撃の経った時間。ティックで数える（結果の画面の「かかった時間」。SimpleRun_Plan.txt Q9・C3）。
// 止めるのは呼ぶ側（一時停止・カバン・漁る画面の間は世界を進めないので tick が来ない＝自然に止まる）。
// 2026-11-09 に時間制限をやめたので、残り時間と時間切れは外した（Q4）。
class RaidClock{
public:
	RaidClock();

	// 1 ティック進める。
	void tick();
	// 始まってからの秒。
	float elapsedSeconds() const;

private:
	int mElapsedTicks;
};

#endif
