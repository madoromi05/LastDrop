#ifndef INCLUDED_RAID_PARAMS_H
#define INCLUDED_RAID_PARAMS_H

#include "GameLib/Math/Vector3.h"

// 出撃と帰還（1 日の時間・脱出点・スコアの重み）の調整値。仕様は SummaryPlan/個別仕様/R2_Plan.txt（Q4・Q9〜Q11）。
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）。
// どれも遊んで確かめる前の初期値（R2_Plan.txt 10 章の「遊んで」で決める）。
struct RaidParams{
	// ── 1 日（1 回の出撃）──
	float raidSeconds = 600.f;       // 1 日の長さ（秒。RaidLoop Q4 の 10 分）。0 になったら行方不明
	float warningSeconds = 60.f;     // 残りがこれ以下になったら、残り時間を赤くする（Q9）
	float exitNoticeSeconds = 5.f;   // 始めに開いている脱出点の名前を出す長さ（秒。Q10）
	// ── 脱出点（Q10・Q11）──
	float extractSeconds = 5.f;      // 円の中にこれだけ居続けたら脱出（RaidLoop Q5）
	float extractRadius = 3.f;       // 脱出点の円の半径（m。水平だけで測る。道の幅 約 7.7m の中に収まる）
	// 開いている脱出点の緑の光（発煙筒の色）。街灯（高さ 5m・橙）と見分けられるよう、低く・緑にする。
	float exitLightHeight = 1.5f;    // 光の高さ（m。車道の面から）
	float exitLightRange = 12.f;     // 届く距離（m）
	GameLib::Math::Vector3 exitLightColor = GameLib::Math::Vector3( 0.25f, 1.f, 0.3f );
	float exitLightIntensity = 40.f;
	// ── スコアの重み（Q4。物の点は ItemSpec::scoreValue）──
	float scorePerDay = 1000.f;      // 生き残った日数 1 日あたり
	float scorePerKill = 20.f;       // 倒したゾンビ 1 体あたり
	float clearBonus = 3000.f;       // 5 日目に脱出したときだけ

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）。
	template< class V > void visit( V& v ) {
		v( "raidSeconds", raidSeconds, 10.f, 1800.f, "s" );
		v( "warningSeconds", warningSeconds, 0.f, 300.f, "s" );
		v( "exitNoticeSeconds", exitNoticeSeconds, 0.f, 20.f, "s" );
		v( "extractSeconds", extractSeconds, 0.5f, 30.f, "s" );
		v( "extractRadius", extractRadius, 0.5f, 10.f, "m" );
		v( "exitLightHeight", exitLightHeight, 0.f, 10.f, "m" );
		v( "exitLightRange", exitLightRange, 1.f, 40.f, "m" ); // 0 は不可（届く距離で割るため）
		v( "exitLightColor", exitLightColor, 0.f, 1.f, "" );
		v( "exitLightIntensity", exitLightIntensity, 0.f, 200.f, "x" );
		v( "scorePerDay", scorePerDay, 0.f, 10000.f, "" );
		v( "scorePerKill", scorePerKill, 0.f, 1000.f, "" );
		v( "clearBonus", clearBonus, 0.f, 20000.f, "" );
	}
};

// 起動中ずっと 1 つだけの調整値（RaidParams.cpp が持つ）。
RaidParams& raidParams();

#endif
