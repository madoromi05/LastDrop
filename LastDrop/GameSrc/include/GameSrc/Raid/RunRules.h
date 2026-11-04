#ifndef INCLUDED_RUN_RULES_H
#define INCLUDED_RUN_RULES_H

#include "GameSrc/Raid/RunState.h"
#include "GameSrc/Raid/DayResult.h"
#include "GameSrc/Raid/Score.h"

// 出撃と帰還・5 日間の決まり（何が残り、何を失い、何点になるか）を書く、ただ 1 か所（R2_Plan.txt 4・5 章）。
// シーン（拠点・本編・結果）はここに頼むだけで、決まりを自分で書かない。状態は持たない（持つのは RunState）。
namespace RunRules{
	// 1 日目の形にする（倉庫は 9mm の弾 48 発・カバンは弾倉いっぱいの拳銃・倒した数 0。R2_Plan.txt Q14・InventoryPickup_Plan.txt Q1）。
	// ハイスコアは残す（Q2）。
	void startNewRun( RunState* run );
	// 保存のファイルを読む。無ければ 1 日目の形で始める（起動して最初に拠点を開くとき。Q16）。
	void loadOrStart( RunState* run );
	// 出撃する（拠点の Deploy）。保存する（この形が「その日のやり直し」の形になる。Q3）。
	// 2026-11-05 にタダの拳銃を渡すのはやめた（始めのカバンに入っている。InventoryPickup_Plan.txt Q1）。
	void deploy( RunState* run );
	// 脱出した（Q1・Q8）。戻ったカバン bagAfterRaid（銃に込めた弾も書き戻した物）をカバンにし、倒した数を足し、日を 1 つ進めて保存する。
	// day: その日の結果（持ち帰った物はここで入れる）。5 日目なら 5 日間を終える（endRun。クリア）。
	void finishDay( RunState* run, const DayResult& day, const ItemGrid& bagAfterRaid );
	// 死んだ・時間切れ（ゲームオーバー）。day: その日の結果。カバンは失う（拠点のカバンは出撃の時の形のままなので、スコアに入れない）。
	void failDay( RunState* run, const DayResult& day );
	// 物の点（ItemSpec::scoreValue × 数。銃に込めた弾も、その弾の点で数える。Q4・Q13）。
	int itemPoints( const ItemGrid& grid );
}

#endif
