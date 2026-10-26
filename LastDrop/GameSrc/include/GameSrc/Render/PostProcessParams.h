#ifndef INCLUDED_POST_PROCESS_PARAMS_H
#define INCLUDED_POST_PROCESS_PARAMS_H

// 画面へ写す処理（PostProcess・Bloom・Present.hlsl）の調整値。仕様は SummaryPlan/個別仕様/LinearToneMap_Plan.txt の 6・7 章。
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）。
// どれもリニア化（2026-10-26）の直後の初期値。画面で見ながら、今までの絵より暗い所がつぶれないように合わせる。
struct PostProcessParams{
	// シーンの色に掛けてから ACES に通す（固定値。自動露出は入れない：暗い所で勝手に明るくなり、夜の暗さが消えるため）。
	float exposure = 1.0f;
	// 明るさ（r, g, b の一番大きい値。リニア）がこれを超えた分だけをにじませる（Bloom_Plan.txt 3 章）。
	float bloomThreshold = 1.0f;
	// にじませた絵を足す強さ。4 つの段を足した絵なので、1 だと超えた分の約 4 倍の光が足される。
	float bloomIntensity = 0.5f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）。
	template< class V > void visit( V& v ) {
		v( "exposure", exposure, 0.05f, 8.f, "x" );
		v( "bloomThreshold", bloomThreshold, 0.f, 8.f, "" );
		v( "bloomIntensity", bloomIntensity, 0.f, 4.f, "x" );
	}
};

// 起動中ずっと 1 つだけの調整値（PostProcess.cpp が持つ）。
PostProcessParams& postProcessParams();

#endif
