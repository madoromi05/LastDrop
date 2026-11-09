#ifndef INCLUDED_RAID_START_H
#define INCLUDED_RAID_START_H

#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Raid/DayCondition.h"

// 出撃の始めの形（始めのカバンと、今回の条件）を決める、ただ 1 か所（SummaryPlan/個別仕様/出撃と帰還/SimpleRun_Plan.txt Q5・Q6・C2・C4）。
// 1 回勝負なので、毎回同じカバン・毎回ランダムな条件で始める。
// 状態は持たない（シーンは出撃のたびにここへ頼むだけ）。
namespace RaidStart{
	const int kBagColumns = 2; // カバンは 2 × 4（8 マス）
	const int kBagRows = 4;

	// 始めのカバン：弾倉いっぱいの拳銃と 9mm の弾 48 発（1 マス 24 発 × 2 マス）。
	ItemGrid startBag();
	// 今回の条件を、ふつう・停電・霧・雨・群れから等しく 1 つ選ぶ。seed: 乱数の種（起動からの時間など）。
	DayCondition rollCondition( unsigned seed );
}

#endif
