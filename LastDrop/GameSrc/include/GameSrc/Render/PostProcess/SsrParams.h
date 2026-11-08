#ifndef INCLUDED_SSR_PARAMS_H
#define INCLUDED_SSR_PARAMS_H

// 映り込み（SSR）の調整値。仕様は SummaryPlan/個別仕様/SsrDecalMaterial_Plan.txt の 7-5・9 章。
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）。
// どれも見た目を確かめる前の初期値。F6 → Render の G-Buffer を「SSR」にすると、映り込みの確からしさが白黒で見える。
struct SsrParams{
	// これより粗い面は計算しない（空と地面の色の映り込みのまま）。水たまり 0.05・車 は画像の値・道路 0.6〜0.7。
	// 0.4 より粗い面は向きをばらしたぼけた映り込み（HiZSsr_Plan.txt 7 章。段階12-b で 0.4 → 0.7）
	float maxRoughness = 0.7f;
	// 反射の向きへ進む歩数（整数に丸めて使う）。多いほど細い物も当たるが、その分だけ重い
	float steps = 24.f;
	// 一番遠くまで進む長さ（m）。町の交差点の間は 33m
	float maxDistance = 30.f;
	// 点が物の奥へこれより深く入ったら、当たりとみなさない（m。薄い物の裏まで当たったことにしない）
	float thickness = 0.5f;
	// 画面の端で薄くする幅（画面に対する割合）。端で映り込みがぷつっと切れないように
	float edgeFade = 0.1f;
	// 全体の強さ（1 で計算どおり。見比べ用）
	float intensity = 1.f;
	// Hi-Z で進む回数の上限（整数に丸めて使う。HiZSsr_Plan.txt 6 章）。足りないと遠くの映り込みが途切れる。直線の進み方では使わない
	float hiZMaxIterations = 48.f;

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）。
	template< class V > void visit( V& v ) {
		v( "maxRoughness", maxRoughness, 0.05f, 1.f, "" );
		v( "steps", steps, 4.f, 64.f, "" );
		v( "maxDistance", maxDistance, 2.f, 100.f, "m" );
		v( "thickness", thickness, 0.05f, 3.f, "m" );
		v( "edgeFade", edgeFade, 0.01f, 0.5f, "" );
		v( "intensity", intensity, 0.f, 2.f, "x" );
		v( "hiZMaxIterations", hiZMaxIterations, 8.f, 128.f, "" );
	}
};

// 起動中ずっと 1 つだけの調整値（ScreenSpaceReflection.cpp が持つ）。
SsrParams& ssrParams();
// 映り込みを作るか（F6 → Render の SSR。見比べと計測用）。false の間は作らず、混ぜるパスも描かない（空と地面の色の映り込みのまま）。
// 起動中ずっと 1 つ（ScreenSpaceReflection.cpp が持つ。初期値 true。Release では true のまま）。
bool& ssrEnabled();
// 粗い面の反射の向きを GGX でばらすか（F6 → Render の Stochastic SSR。HiZSsr_Plan.txt 7・8 章）。false の間は鏡の向きだけで、ぼかしも今までのまま。
// 起動中ずっと 1 つ（ScreenSpaceReflection.cpp が持つ。初期値 true。Release では true のまま）。
bool& ssrStochasticEnabled();

#endif
