#ifndef INCLUDED_RAID_RESULT_H
#define INCLUDED_RAID_RESULT_H

// 1 回の出撃の終わり方（SummaryPlan/個別仕様/出撃と帰還/SimpleRun_Plan.txt）。
// 時間制限が無いので、時間切れ（行方不明）の終わり方は無い。
enum class RaidEnding{
	None,    // まだ終わっていない
	Escaped, // 脱出の車で町を出た（クリア）
	Killed,  // 死んだ
};

// 1 回の出撃がどう終わったか（結果の画面 RaidEndScene が読む）。作るのは RaidSession、次のシーンへ渡すのは RaidRecord。
struct RaidResult{
	RaidEnding ending = RaidEnding::None;
	float seconds = 0.f; // 出撃してからの秒（一時停止・カバン・漁る画面の間は数えない）
	int kills = 0;       // 倒したゾンビの数
};

#endif
