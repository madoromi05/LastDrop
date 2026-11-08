#ifndef INCLUDED_DAY_RESULT_H
#define INCLUDED_DAY_RESULT_H

#include "GameSrc/Interaction/Item/ItemGrid.h"
#include <vector>

// 1 日（1 回の出撃）の終わり方（R2_Plan.txt Q1・Q12）。
enum class RaidEnding{
	None,      // まだ終わっていない
	Survived,  // 脱出点から出た（日が進む）
	Killed,    // 死んだ（その場でゲームオーバー）
	Missing,   // 時間切れで行方不明（死んだのと同じ）
};

// 1 日がどう終わったか（1 日の結果の画面が読む）。作るのは RaidSession と GameScene、持つのは RunState。
struct DayResult{
	int day = 0;                         // 何日目か（1〜）
	RaidEnding ending = RaidEnding::None;
	float seconds = 0.f;                 // 出撃してからの秒
	int kills = 0;                       // この日に倒したゾンビの数
	std::vector< ItemStack > broughtBack; // 持ち帰った物（カバンの中身。脱出したときだけ）
};

#endif
