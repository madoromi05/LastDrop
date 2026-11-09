#ifndef INCLUDED_TAA_PARAMS_H
#define INCLUDED_TAA_PARAMS_H

// TAA の調整値。仕様は SummaryPlan/個別仕様/グラフィック/Taa_Plan.txt の 7-5 章。
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）。
// どれも見た目を確かめる前の初期値。履歴を混ぜる割合を上げるほど縁はなめらかになるが、ぼけと残像が出やすい。
// 箱の広さを下げるほど残像は消えるが、細い物のちらつきが戻る。
struct TaaParams{
	// 履歴を混ぜる割合（0〜1。0.9 なら新しい色は 1 割ずつ入る）。動きの種類ごと（Taa_Plan.txt 6 章）
	float historyWeight = 0.9f;            // 動かない物（建物・車・置物・地面）
	float movingHistoryWeight = 0.8f;      // 動く物（ゾンビ・千切れた手足）。前の位置が正確でないので、新しい色を多めに入れる
	float firstPersonHistoryWeight = 0.85f; // 手元（自キャラ・銃・弾）。画面の上で少し揺れる（照準のぶれ・歩きの揺れ）分だけ新しい色を多めに
	// 履歴を寄せる箱の広さ（周り 3×3 の色の標準偏差の何倍か）。動きの種類ごと
	float clipGamma = 1.f;
	float movingClipGamma = 0.75f;          // 動く物は狭く（通った後ろに残像を残さない）
	float firstPersonClipGamma = 1.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）。
	template< class V > void visit( V& v ) {
		v( "historyWeight", historyWeight, 0.f, 0.98f, "" );
		v( "movingHistoryWeight", movingHistoryWeight, 0.f, 0.98f, "" );
		v( "firstPersonHistoryWeight", firstPersonHistoryWeight, 0.f, 0.98f, "" );
		v( "clipGamma", clipGamma, 0.25f, 3.f, "" );
		v( "movingClipGamma", movingClipGamma, 0.25f, 3.f, "" );
		v( "firstPersonClipGamma", firstPersonClipGamma, 0.25f, 3.f, "" );
	}
};

// 起動中ずっと 1 つだけの調整値（TemporalAntiAliasing.cpp が持つ）。
TaaParams& taaParams();

#endif
