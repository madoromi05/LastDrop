// 画素の位置だけで決まるノイズ（テクスチャ不要）。ディザ（Present.hlsl。NightFinish_Plan.txt 7 章）と
// SSAO の点の並びを画素ごとに回す角度（Ssao.hlsl。SsaoTranslucent_Plan.txt 5-2）が同じ物を使う。
#ifndef INCLUDED_NOISE_HLSLI
#define INCLUDED_NOISE_HLSLI

// Interleaved Gradient Noise（Jorge Jimenez, 2014）。画素の位置だけで決まる 0〜1 のノイズ（テクスチャ不要）。
// 隣の画素と値が大きく違い、なだらかな色の上でも模様が目立たない。フレームごとには動かさない（TAA が無いので、動かすとちらつく）。
static const float3 kNoiseScale = float3(0.06711056, 0.00583715, 52.9829189);
float interleavedGradientNoise(float2 pixel)
{
    return frac(kNoiseScale.z * frac(dot(pixel, kNoiseScale.xy)));
}

// フレームごとに動かす版（TAA のとき。SummaryPlan/個別仕様/Taa_Plan.txt 8 章）。画素の位置をフレームの番号 × 5.588238 だけずらす
// （Jimenez 2014 の方法。この歩幅だと、続くフレームで同じ画素の値が重ならずに散る）。TAA が何フレームかを均すので、同じ点の数でざらつきが減る。
// frame：TemporalJitter::frameIndex（TAA でないときは 0 で、上の版と同じ値）。
static const float kNoiseFrameStep = 5.588238;
float interleavedGradientNoise(float2 pixel, float frame)
{
    return interleavedGradientNoise(pixel + frame * kNoiseFrameStep);
}

#endif
