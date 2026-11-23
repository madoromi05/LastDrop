#ifndef INCLUDED_RAID_RECORD_H
#define INCLUDED_RAID_RECORD_H

#include "GameSrc/Raid/RaidResult.h"
#include "GameSrc/Raid/DayCondition.h"

// 出撃（GameScene）と結果の画面（RaidEndScene）の間で受け渡す物
// 起動から終了まで 1 つだけ（GameResources が持つ）。シーンは作り直すたびに消えるので、次のシーンへ渡す物はここに置く
// ・最後の出撃の結果（結果の画面が読む）
// ・次の出撃の条件の固定（F6。確かめたい条件をすぐ出すため。固定していなければ毎回ランダム）
// ・次の 1 回の出撃だけの、日の条件と脱出の車の置き場所（F6 の Town タブが、同じ条件・決めた車のまま町を読み直すため）
//   次に作る出撃が takeRaidChoice で使って消す。条件の固定とは別に持つので、読み直した後に F6 の設定を戻す必要が無い
// 保存はしない（回をまたいで残る物は無い）
class RaidRecord{
public:
	// 出撃 1 回の始めに決める物
	struct RaidChoice{
		DayCondition condition;	// その日の条件
		int escapeCarSpot;		// 脱出の車の置き場所（TownMap::escapeCarSpots の番号）。負なら RaidSession が乱数で選ぶ
	};

	RaidRecord();

	const RaidResult& lastResult() const;
	void setLastResult( const RaidResult& result );

	// 次の出撃の日の条件と脱出の車の置き場所を決める。出撃（GameScene）を作るときに 1 回だけ呼ぶ。seed: 日の条件の乱数の種
	// 次の 1 回だけの決め事（overrideNextRaid）があれば、それを返して消す
	// 無ければ、条件は固定していればそれ・無ければ RaidStart::rollCondition。車の置き場所は負
	RaidChoice takeRaidChoice( unsigned seed );
	// ── F6（Debug・Profile だけで呼ぶ）──
	bool hasForcedCondition() const;
	DayCondition forcedCondition() const;
	void forceCondition( DayCondition condition );
	void clearForcedCondition();
	void overrideNextRaid( DayCondition condition, int carSpot );	// 次の 1 回の出撃だけ、条件と車の置き場所を決める。条件の固定は変えない。carSpot: 0 以上
	bool hasNextRaidOverride() const;								// overrideNextRaid の後、まだ出撃が使っていないか
	void dropNextRaidOverride();									// 使わずに捨てる（次に開いたシーンが出撃でなかったとき）

private:
	RaidResult mLastResult;
	bool mHasForcedCondition;
	DayCondition mForcedCondition;
	bool mHasNextRaidOverride;
	RaidChoice mNextRaidOverride;	// mHasNextRaidOverride のときだけ読む
};

#endif
