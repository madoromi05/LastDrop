// 画面へ写す処理の定数（PostProcess.cpp の PresentConstants と同じ並び）。Present.hlsl と、色の表を焼く LutBake.hlsl が読む
// 色の調整の値を両方が同じ所から読むので、焼いた表と画面の色がずれない（MoonStarsLut_Plan.txt Q4）
#ifndef INCLUDED_PRESENT_CONSTANTS_HLSLI
#define INCLUDED_PRESENT_CONSTANTS_HLSLI

cbuffer PresentConstants : register(b1) // PostProcess.cpp の PresentConstants と同じ並び
{
    float gVignetteStrength; // 角での暗くなり具合（0で無し、1で真っ黒）
    float gVignetteInner;    // 暗くなり始める距離（画面中心0〜上下左右の端1）
    float gVignetteOuter;    // 一番暗くなる距離（角は約1.41）
    float gBloomIntensity;   // ブルームを足す強さ
    float gDamageFlash;      // 噛まれた瞬間の赤みの強さ（0で無し、1で一番赤い）
    float gFlashInner;       // 赤くなり始める距離（画面中心0〜上下左右の端1）
    float gFlashOuter;       // 一番赤くなる距離
    float gFade;             // 画面全体を暗くする量（0でそのまま、1で真っ黒。死んだ後の暗転）
    float3 gFlashColor;      // 赤みの色（画面の色。混ぜる前にリニアへ直す）
    float gExposure;         // トーンマップの前にシーンへ掛ける倍率（PostProcessParams::exposure）
    float gPain;             // 噛まれた瞬間の痛みの強さ（0で無し、1で一番強い）。周りのぼかしと暗さ
    float gPainBlurLength;   // 痛み1・画面の端で、中心までの距離の何割の長さを読んでぼかすか
    float gPainBlurInner;    // ぼかし始める距離（画面中心0〜上下左右の端1）。これより内（照準の周り）はぼかさない
    float gPainDarken;       // 痛み1のとき、ビネットの暗さに足す量
    // ── カラーグレーディングとディザ（PostProcessParams） ──
    float3 gWhiteBalance;    // 色温度から作った r・g・b の倍率（明るさが変わらないよう割り戻し済み）
    float gSaturation;       // 彩度（1 でそのまま）
    float gContrast;         // コントラスト（中間の灰色を中心に。1 でそのまま）
    float gShadowsEnd;       // 暗部の重みが 0 になる明るさ（画面の明るさ）
    float gHighlightsStart;  // 明部の重みが付き始める明るさ
    float gNightDesaturation; // 暗い所ほど彩度を下げる量（0 で無し）
    float3 gShadowsColor;    // 暗部に掛ける色（sRGB。白でそのまま）
    float gDitherAmount;     // 足すノイズの幅（画面の色の 1 段。0 で無し）
    float3 gMidtonesColor;   // 中間に掛ける色
    float gNoiseFrame;       // ディザのノイズのフレームの番号（TAA のとき毎フレーム変わる。それ以外は 0）
    float3 gHighlightsColor; // 明部に掛ける色
    float gLutStrength;      // 色の表（LUT）を混ぜる強さ（0 で引かない。Grading.hlsli の applyLut）
};

#endif
