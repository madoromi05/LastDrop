#ifndef INCLUDED_POST_PROCESS_PARAMS_H
#define INCLUDED_POST_PROCESS_PARAMS_H

#include "GameLib/Math/Vector3.h"

// 画面へ写す処理（PostProcess・Bloom・Present.hlsl）の調整値。仕様は SummaryPlan/個別仕様/LinearToneMap_Plan.txt の 6・7 章、
// カラーグレーディングは NightFinish_Plan.txt 6 章、FXAA は 8 章。
// 起動中に F6 のエディタ（Tuning）で動かせる。決まった値は Copy as C++ でここの初期値へ戻す（コードの値が正）。
// どれもリニア化（2026-10-26）の直後の初期値。画面で見ながら、今までの絵より暗い所がつぶれないように合わせる。
struct PostProcessParams{
	// シーンの色に掛けてから ACES に通す（固定値。自動露出は入れない：暗い所で勝手に明るくなり、夜の暗さが消えるため）。
	float exposure = 1.0f;
	// 明るさ（r, g, b の一番大きい値。リニア）がこれを超えた分だけをにじませる（Bloom_Plan.txt 3 章）。
	float bloomThreshold = 1.0f;
	// にじませた絵を足す強さ。4 つの段を足した絵なので、1 だと超えた分の約 4 倍の光が足される。
	float bloomIntensity = 0.5f;

	// ── カラーグレーディング（NightFinish_Plan.txt 6 章）── ACES の後の 0〜1 の色に、上から順に掛ける。
	// 初期値はどれも「何も変えない」値。画面で決める順番も上から（全体を決めてから細部）。色は画像の色と同じ数（sRGB）。
	float temperature = 0.f;       // 色温度（負で青、正で黄。明るさは変えない）
	float saturation = 1.f;        // 彩度（0 で白黒、1 でそのまま）
	float nightDesaturation = 0.f; // 暗い所ほど彩度を下げる量（0 で無し。人の目が暗い所で色を感じにくくなるのに似せる）
	float contrast = 1.f;          // コントラスト（リニアの中間の灰色 0.18 を中心に。1 でそのまま）
	GameLib::Math::Vector3 shadowsColor = GameLib::Math::Vector3( 1.f, 1.f, 1.f );    // 暗部に掛ける色（白でそのまま）
	GameLib::Math::Vector3 midtonesColor = GameLib::Math::Vector3( 1.f, 1.f, 1.f );   // 中間に掛ける色
	GameLib::Math::Vector3 highlightsColor = GameLib::Math::Vector3( 1.f, 1.f, 1.f ); // 明部に掛ける色
	// 暗部・明部の境目。画面の明るさ（sRGB の数の輝度）で測る（夜の絵はリニアの数だとほとんどが暗部になるため）。
	float shadowsEnd = 0.3f;       // 暗部の重みが 0 になる明るさ（Unity URP の Shadows Midtones Highlights の初期値と同じ）
	float highlightsStart = 0.55f; // 明部の重みが付き始める明るさ（同上）

	// ── 色の表（LUT。MoonStarsLut_Plan.txt 6 章）── Resource/UI/lut.tga を読んでいる間だけ効く。グレーディングの後の色を、表で引いた色へ寄せる割合
	float lutStrength = 1.f;       // 0 で gradeColor のまま、1 で表の色（タルコフの色のフィルタの強さと同じ扱い）

	// ── FXAA（NightFinish_Plan.txt 8 章）── 輝度の差で輪郭を見つけ、輪郭を横切る向きに隣の画素と混ぜる
	float fxaaEdgeThreshold = 0.125f;     // 輪郭とみなす輝度の差の割合（周りで一番明るい輝度に対して。FXAA の推奨の高い品質）
	float fxaaEdgeThresholdMin = 0.0312f; // 暗い所で輪郭とみなす差の下限（FXAA の推奨で一番細かい値。夜の絵は暗いので、暗い建物の輪郭を飛ばさない）
	float fxaaSubpixel = 0.5f;            // 細かい模様（1 画素の点や線）をぼかす量（0 で無し、1 で一番強い。FXAA の初期値 0.75 より弱く）

	// 項目ごとに v( 名前, 値, 下限, 上限, 単位 ) を呼ぶ（AimSwayParams と同じ約束）。
	template< class V > void visit( V& v ) {
		v( "exposure", exposure, 0.05f, 8.f, "x" );
		v( "bloomThreshold", bloomThreshold, 0.f, 8.f, "" );
		v( "bloomIntensity", bloomIntensity, 0.f, 4.f, "x" );
		v( "temperature", temperature, -1.f, 1.f, "" );
		v( "saturation", saturation, 0.f, 2.f, "x" );
		v( "nightDesaturation", nightDesaturation, 0.f, 1.f, "" );
		v( "contrast", contrast, 0.5f, 2.f, "x" );
		v( "shadowsColor", shadowsColor, 0.f, 1.f, "" );
		v( "midtonesColor", midtonesColor, 0.f, 1.f, "" );
		v( "highlightsColor", highlightsColor, 0.f, 1.f, "" );
		v( "shadowsEnd", shadowsEnd, 0.01f, 1.f, "" );       // 0 は不可（境目で割るため）
		v( "highlightsStart", highlightsStart, 0.f, 0.99f, "" ); // 1 は不可（同上）
		v( "lutStrength", lutStrength, 0.f, 1.f, "" );
		v( "fxaaEdgeThreshold", fxaaEdgeThreshold, 0.03f, 0.5f, "" );
		v( "fxaaEdgeThresholdMin", fxaaEdgeThresholdMin, 0.f, 0.1f, "" );
		v( "fxaaSubpixel", fxaaSubpixel, 0.f, 1.f, "" );
	}
};

// 起動中ずっと 1 つだけの調整値（PostProcess.cpp が持つ）。
PostProcessParams& postProcessParams();
// カラーグレーディング・ディザを掛けるか（F6 → Render。見比べと計測用。NightFinish_Plan.txt 9・11 章）。
// false の間は「何も変えない」値をシェーダへ渡す。起動中ずっと 1 つ（PostProcess.cpp が持つ。初期値 true。Release では true のまま）。
bool& colorGradingEnabled();
bool& ditherEnabled();
// 色の表（LUT）の F6 のボタンと表示（MoonStarsLut_Plan.txt 7・8 章）。ボタンが要求を立て、PostProcess が次の endScene で行って下ろす。
// 起動中ずっと 1 つ（PostProcess.cpp が持つ）。
struct ColorLutControl{
	bool exportRequested = false; // 今の色の調整を Resource/UI/lut_export.tga に書き出す
	bool reloadRequested = false; // Resource/UI/lut.tga を読み直す
	bool loaded = false;          // 今 lut.tga を引けるか（PostProcess が毎フレーム書く。F6 の表示用）
};
ColorLutControl& colorLutControl();
// アンチエイリアスの方式（F6 → Render。タルコフと同じく 1 つから選ぶ。Taa_Plan.txt Q4）。
//   Off  … 何も掛けない
//   Fxaa … 写した後に FXAA（NightFinish_Plan.txt 8 章）。Fxaa 以外の間は 8bit の中間を通さず、画面へ直接写す（gpu.FXAA はほぼ 0 になる）
//   Taa  … 描く行列を毎フレームずらし（TemporalJitter）、TAA で混ぜる（Taa_Plan.txt）。FXAA は通さない（ぼけが重なるため）
enum class AntiAliasing{
	Off,
	Fxaa,
	Taa,
	Count,
};
// エディタ（F6 の Render）に出す名前。
const char* antiAliasingName( AntiAliasing );
// 起動中ずっと 1 つ（PostProcess.cpp が持つ。初期値は Taa。Release では初期値のまま）。
AntiAliasing& antiAliasingMode();

#endif
