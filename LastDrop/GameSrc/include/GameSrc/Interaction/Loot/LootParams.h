#ifndef INCLUDED_LOOT_PARAMS_H
#define INCLUDED_LOOT_PARAMS_H

// 漁れる場所（ゴミ箱・木箱）と包帯の調整値（開けるのにかかる秒・空になる確率・包帯の効き目）
// 起動中に F6 のエディタ（Tuning の「Loot」）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）
// 場所ごとに出やすい物（重み・数）は LootTable.cpp の表（データ）。どれも遊んで確かめる前の初期値
struct LootParams{
	// ── 開けるのにかかる秒──
	float dumpsterSeconds = 1.5f; // ゴミ箱
	float crateSeconds = 2.5f;    // 木箱
	// ── 空になる確率（0〜1。開けてみないと分からない）
	float dumpsterEmptyChance = 0.1f; // ゴミ箱
	float crateEmptyChance = 0.1f;    // 木箱
	// ── 包帯（体力は 100。35 で Danger → Caution、2 つで Fine に戻る） ──
	float bandageHeal = 35.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（RaidParams と同じ約束）
	template< class V > void visit( V& v ) {
		v( "dumpsterSeconds", dumpsterSeconds, 0.f, 10.f, "s" );
		v( "crateSeconds", crateSeconds, 0.f, 10.f, "s" );
		v( "dumpsterEmptyChance", dumpsterEmptyChance, 0.f, 1.f, "" );
		v( "crateEmptyChance", crateEmptyChance, 0.f, 1.f, "" );
		v( "bandageHeal", bandageHeal, 0.f, 100.f, "" );
	}
};

// 起動中ずっと 1 つだけの調整値
LootParams& lootParams();

#endif
