#ifndef INCLUDED_RAID_RECORD_H
#define INCLUDED_RAID_RECORD_H

#include "GameSrc/Raid/RaidResult.h"
#include "GameSrc/Raid/DayCondition.h"

// 出撃（GameScene）と結果の画面（RaidEndScene）の間で受け渡す物（SummaryPlan/個別仕様/出撃と帰還/SimpleRun_Plan.txt）。
// 起動から終了まで 1 つだけ（GameResources が持つ）。シーンは作り直すたびに消えるので、次のシーンへ渡す物はここに置く。
// ・最後の出撃の結果（結果の画面が読む）
// ・次の出撃の条件の固定（F6。確かめたい条件をすぐ出すため。固定していなければ毎回ランダム。C9）
// 保存はしない（回をまたいで残る物は無い。Q10）。2026-11-09 に 5 日間の様子（RunState）から作り直した。
class RaidRecord{
public:
	RaidRecord();

	const RaidResult& lastResult() const;
	void setLastResult( const RaidResult& result );

	// 次の出撃の条件を決める（固定していればそれ、無ければ RaidStart::rollCondition）。seed: 乱数の種。
	DayCondition chooseCondition( unsigned seed ) const;
	// ── F6（Debug・Profile だけで呼ぶ）──
	bool hasForcedCondition() const;
	DayCondition forcedCondition() const;
	void forceCondition( DayCondition condition );
	void clearForcedCondition();

private:
	RaidResult mLastResult;
	bool mHasForcedCondition;
	DayCondition mForcedCondition;
};

#endif
