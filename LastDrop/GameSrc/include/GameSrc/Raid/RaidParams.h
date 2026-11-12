#ifndef INCLUDED_RAID_PARAMS_H
#define INCLUDED_RAID_PARAMS_H

// 出撃（脱出の車・ガソリン・始めの知らせ）の調整値。仕様は SummaryPlan/個別仕様/出撃と帰還/FuelEscapeLoading_Plan.txt
// （脱出の車とガソリン）。時間制限とスコアは無い
// 小数の値は起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）
// どれも遊んで確かめる前の初期値
struct RaidParams{
	// ── ガソリンの数（数を変えるときはここだけ直す） ──
	// 出撃の始めにしか効かないので F6 には出さない（Tuning は小数だけ）
	int requiredFuel = 1;	// 脱出に要るガソリンの数（脱出の車 1 台に入れる数。1 以上）
	int fuelCanCount = 2;	// 町に置くガソリンの数（漁れる場所の箱に 1 個ずつ入れる。箱の数より多くはできない＝ASSERT）
	int rifleCount = 1;		// 町に置くライフルの数（木箱に 1 丁ずつ・弾倉いっぱいで必ず入れる。木箱の数より多くはできない＝ASSERT）

	// ── 1 回の出撃 ──
	float exitNoticeSeconds = 5.f;   // 始めに今回の条件と脱出の車の場所（「脱出の車：北」）を出す長さ（秒）
	// ── 脱出の車 ──
	float refuelSeconds = 2.f;       // ガソリン 1 個を入れる長さ（秒。動く・撃つ・構える・攻撃で取りやめ。漁るのと同じ）
	float extractSeconds = 5.f;      // 給油し終えた車のそばにこれだけ居続けたら脱出
	float extractRadius = 4.f;       // 車のそばの円の半径（m。車の中心から水平だけで測る。車体 約 4m の長さの外まで届く）

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）
	template< class V > void visit( V& v ) {
		v( "exitNoticeSeconds", exitNoticeSeconds, 0.f, 20.f, "s" );
		v( "refuelSeconds", refuelSeconds, 0.1f, 10.f, "s" );
		v( "extractSeconds", extractSeconds, 0.5f, 30.f, "s" );
		v( "extractRadius", extractRadius, 0.5f, 10.f, "m" );
	}
};

RaidParams& raidParams();	// 起動中ずっと 1 つだけの調整値（RaidParams.cpp が持つ）

#endif
