#ifndef INCLUDED_RAID_CLOCK_H
#define INCLUDED_RAID_CLOCK_H

// 1 日（1 回の出撃）の残り時間。ティックで数える（R2_Plan.txt Q9）。
// 止めるのは呼ぶ側（一時停止・カバンの間は世界を進めないので tick が来ない＝自然に止まる）。
// 何秒にするかは作るときに受け取る（RaidParams::raidSeconds）。時間が切れたら何が起きるかは知らない（RaidSession が決める）。
class RaidClock{
public:
	// seconds: 1 日の長さ（秒。0 より大きいこと）。
	explicit RaidClock( float seconds );

	// 1 ティック進める（切れた後は 0 のまま）。
	void tick();
	// 残りの秒（0 以上）。
	float remainingSeconds() const;
	// 始まってからの秒（結果の画面の「Time」）。
	float elapsedSeconds() const;
	bool isExpired() const;
	// 残りを seconds にする（F6 で、時間切れをすぐ試すため）。0 以上。
	void setRemainingSeconds( float seconds );

private:
	int mTotalTicks;
	int mRemainingTicks;
};

#endif
