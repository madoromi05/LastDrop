#ifndef INCLUDED_SCORE_H
#define INCLUDED_SCORE_H

#include "GameSrc/Raid/DayResult.h"

// 5 日間の終わり（クリアかゲームオーバー）に出すスコアの内訳（R2_Plan.txt Q4・Q5）。数えるのは RunRules::endRun。
// 重みは RaidParams（scorePerDay・scorePerKill・clearBonus）、物の点は ItemSpec::scoreValue。
struct Score{
	int daysSurvived = 0; // 生き残った日数（脱出できた日の数）
	int kills = 0;        // 5 日間に倒した数（死んだ日の分も入れる）
	int dayPoints = 0;    // 日数 × scorePerDay
	int killPoints = 0;   // 倒した数 × scorePerKill
	int itemPoints = 0;   // 持ち帰った物の点（終わった時の倉庫とカバン。死んだ日のカバンは入れない）
	int clearBonus = 0;   // 5 日目に脱出したときだけ

	int total() const { return dayPoints + killPoints + itemPoints + clearBonus; }
};

// 5 日間がどう終わったか（クリア・ゲームオーバーの画面が読む）。
struct RunEnd{
	RaidEnding ending = RaidEnding::None; // Survived ならクリア（5 日目に脱出した）。Killed・Missing はゲームオーバー
	int day = 0;                          // 終わった日（何日目か）
	Score score;
	bool newBest = false;                 // ハイスコアを超えたか
};

#endif
