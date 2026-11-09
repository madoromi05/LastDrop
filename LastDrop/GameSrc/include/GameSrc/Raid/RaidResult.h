#ifndef INCLUDED_RAID_RESULT_H
#define INCLUDED_RAID_RESULT_H

// 1 回の出撃の終わり方（SummaryPlan/個別仕様/出撃と帰還/SimpleRun_Plan.txt）。
// 2026-11-09 に 5 日間の 1 日の結果（DayResult）から作り直した。時間切れ（行方不明）は時間制限と一緒にやめた（Q4）。
enum class RaidEnding{
	None,    // まだ終わっていない
	Escaped, // 脱出の車で町を出た（クリア）
	Killed,  // 死んだ
};

// 1 回の出撃がどう終わったか（結果の画面 RaidEndScene が読む）。作るのは RaidSession、次のシーンへ渡すのは RaidRecord。
struct RaidResult{
	RaidEnding ending = RaidEnding::None;
	float seconds = 0.f; // 出撃してからの秒（一時停止・カバン・漁る画面の間は数えない。C3）
	int kills = 0;       // 倒したゾンビの数
};

#endif
