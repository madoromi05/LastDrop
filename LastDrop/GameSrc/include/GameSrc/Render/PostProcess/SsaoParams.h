#ifndef INCLUDED_SSAO_PARAMS_H
#define INCLUDED_SSAO_PARAMS_H

// SSAO（物の隙間の暗がり）の調整値。仕様は SummaryPlan/個別仕様/グラフィック/SsaoTranslucent_Plan.txt の 5-4・8 章
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）
// どれも見た目を確かめる前の初期値。F6 → Render の G-Buffer を「SSAO」にして、白黒の絵で決める
struct SsaoParams{
	float radius = 0.5f;		// 調べる半球の半径（m）。人の足元・車の下の隙間の大きさ。大きいほど広く暗くなるが、離れた物どうしでも暗くなる
	float sampleCount = 12.f;	// 調べる点の数（整数に丸めて使う）。多いほどざらつきが減るが、その分だけ重い
	float bias = 0.03f;			// 遮られたとみなす奥行きの差の下限（m）。平らな面が自分を遮るしみ（深度の細かさの誤差）を防ぐ
	float intensity = 1.f;		// 遮られた量に掛ける倍率。1 で数えたまま
	float power = 1.5f;			// AO の曲がり（AO を何乗するか）。大きいほど隅が濃くなる

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）
	template< class V > void visit( V& v ) {
		v( "radius", radius, 0.05f, 3.f, "m" );
		v( "sampleCount", sampleCount, 4.f, 32.f, "" );
		v( "bias", bias, 0.f, 0.3f, "m" );
		v( "intensity", intensity, 0.f, 4.f, "x" );
		v( "power", power, 0.5f, 4.f, "" );
	}
};

SsaoParams& ssaoParams();	// 起動中ずっと 1 つだけの調整値（AmbientOcclusion.cpp が持つ）
// SSAO を作るか（F6 → Render の SSAO。見比べと計測用）。false の間は作らず、結果は AO 1（遮られていない）にする
// 起動中ずっと 1 つ（AmbientOcclusion.cpp が持つ。初期値 true。Release では true のまま）
bool& ssaoEnabled();
// SSAO をぼかすか（F6 → Render の SSAO blur。ぼかす前のざらつきを見る調整用）
// 起動中ずっと 1 つ（AmbientOcclusion.cpp が持つ。初期値 true。Release では true のまま）
bool& ssaoBlurEnabled();

#endif
