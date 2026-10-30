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

#endif
