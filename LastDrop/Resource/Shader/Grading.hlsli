// カラーグレーディング（夜の色）と色の表（LUT）。Present.hlsl が画面へ写すときに、LutBake.hlsl が表を焼くときに使う。
// 値は PresentConstants.hlsli（PostProcessParams）。順番と理由は NightFinish_Plan.txt 6 章・MoonStarsLut_Plan.txt 6 章。
#ifndef INCLUDED_GRADING_HLSLI
#define INCLUDED_GRADING_HLSLI

#include "Common.hlsli"
#include "PresentConstants.hlsli"

// ── カラーグレーディング（NightFinish_Plan.txt 6 章）──
// 「色 → 色」の関数 1 つにまとめておく。LutBake.hlsl がこの関数を色の表に焼く（MoonStarsLut_Plan.txt 6-3）。
// リニアの色の輝度の重み（Rec.709。PostProcess.cpp の kLuminanceWeights と同じ）。
static const float3 kLuminanceWeights = float3(0.2126, 0.7152, 0.0722);
// コントラストの中心。0〜1 の真ん中 0.5 ではなく、リニアの中間の灰色（暗い夜の絵がまとめてつぶれないように）。
static const float kMiddleGray = 0.18;
// この画面の明るさより暗い所ほど、gNightDesaturation の分だけ彩度を下げる。
static const float kNightLuminance = 0.25;
// 中間の重みの境目（Unity URP の Shadows Midtones Highlights と同じく、明部の重みは明るさ 1 で 1 になる）。
static const float kShadowsStart = 0.0;
static const float kHighlightsEnd = 1.0;

float3 gradeColor(float3 color)
{
    color *= gWhiteBalance;
    float luminance = dot(color, kLuminanceWeights);
    // 暗部・中間・明部の重みと夜の彩度は、画面の明るさ（sRGB の数）で測る。
    float lightness = linearToSrgb(luminance.xxx).x;
    float saturation = gSaturation * (1.0 - gNightDesaturation * (1.0 - smoothstep(0.0, kNightLuminance, lightness)));
    color = max(lerp(luminance.xxx, color, saturation), 0.0);
    color = kMiddleGray * pow(color / kMiddleGray, gContrast);
    float shadows = 1.0 - smoothstep(kShadowsStart, gShadowsEnd, lightness);
    float highlights = smoothstep(gHighlightsStart, kHighlightsEnd, lightness);
    float midtones = saturate(1.0 - shadows - highlights);
    float3 tint = srgbToLinear(gShadowsColor) * shadows + srgbToLinear(gMidtonesColor) * midtones
                + srgbToLinear(gHighlightsColor) * highlights;
    return saturate(color * tint);
}

// ── 色の表（LUT。MoonStarsLut_Plan.txt 6 章）──
// 1024×32 の帯：32×32 の表を横に 32 枚並べる。横の位置が赤、縦（上から）が緑、何枚目かが青。
// 表の中身も引く色も画面の色（sRGB の数。0〜1）。夜の暗い色に段を多く割り当てるためと、外の道具で画面の写真に掛けた色の調整を
// そのまま表に掛けられるようにするため（Q3）。
// PostProcess.cpp が lut.tga を読んだときだけ t4 に付ける（gLutStrength が 0 の間は読まない）。
Texture2D gLut : register(t4);

static const float kLutSize = 32.0; // 1 辺の段の数（PostProcess.cpp の kLutSize と揃える）

// 恒等の表（何も変えない表）の、画素 pixel（帯の上の位置。整数）にあたる画面の色。表を焼くときに使う。
float3 lutIdentityColor(uint2 pixel)
{
    uint size = (uint)kLutSize;
    return float3(pixel.x % size, pixel.y, pixel.x / size) / (kLutSize - 1.0);
}

// リニアの色 color を表で引いて、リニアの色で返す。
float3 applyLut(float3 color)
{
    float3 screen = linearToSrgb(color); // 0〜1 に収めてから引く
    // 青は何枚目か。隣り合う 2 枚を読んで混ぜる（3D テクスチャの補間の代わり）。
    float blue = screen.b * (kLutSize - 1.0);
    float slice = floor(blue);
    float nextSlice = min(slice + 1.0, kLutSize - 1.0);
    // 赤・緑は 1 枚の中で線形補間。段の中心（端から半画素）を読むので、隣の枚の色は混ざらない。
    float2 inSlice = (screen.rg * (kLutSize - 1.0) + 0.5) / float2(kLutSize * kLutSize, kLutSize);
    float3 low = gLut.SampleLevel(gClampSampler, inSlice + float2(slice / kLutSize, 0.0), 0).rgb;
    float3 high = gLut.SampleLevel(gClampSampler, inSlice + float2(nextSlice / kLutSize, 0.0), 0).rgb;
    return srgbToLinear(lerp(low, high, blue - slice));
}

// 写すときの色の調整。gradeColor を掛け、表を読んでいる間は、表で引いた色（グレーディングを焼いた上に外で直した色）へ強さの分だけ寄せる。
// 強さ 0 なら gradeColor の結果のまま（MoonStarsLut_Plan.txt 6-4）。
float3 gradeColorWithLut(float3 color)
{
    float3 graded = gradeColor(color);
    [branch]
    if (gLutStrength > 0.0)
    {
        graded = lerp(graded, applyLut(color), gLutStrength);
    }
    return graded;
}

#endif
