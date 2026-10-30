// 映り込み（SSR。SsrDecalMaterial_Plan.txt 7 章）の作る・ぼかす・混ぜるパスの共通の式。
// SSR の結果は画面の半分の大きさ（RGBA16F）で、rgb は「映る色 × 確からしさ」、a は確からしさ（ぼかしても色と確からしさがずれないよう、
// 確からしさを掛けた色で持つ）。半分の大きさの画素 i は、元の画面の画素 2i の値（SSAO と同じ。Ssao.hlsl）。
// 行列はパスごとに別の定数に入っているので、ここの関数は行（float4 × 4）を引数で受ける。
#ifndef INCLUDED_SSR_COMMON_HLSLI
#define INCLUDED_SSR_COMMON_HLSLI

#include "DepthPosition.hlsli"
#include "SsaoUpsample.hlsli"

static const float kSsrSkyDepth = 10000.0;  // 空の画素の奥行き（どの物とも近くないと分かる大きさ。Ssao.hlsl の kSkyDistance と同じ）
static const uint kSsrHalfStep = 2;         // 半分の大きさの 1 画素 = 元の画面の 2 画素

// ワールドの位置を、射影×ビュー（行）で画面の uv と奥行き（w）へ。w が 0 以下（目の後ろ）なら uv は使えない。
float3 ssrProjectToScreen(float4 projectionView[4], float3 world)
{
    float4 p = float4(world, 1.0);
    float4 clip = float4(dot(projectionView[0], p), dot(projectionView[1], p), dot(projectionView[2], p), dot(projectionView[3], p));
    float2 ndc = clip.xy / clip.w;
    return float3(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5, clip.w);
}

// 元の画面の画素 pixel に見えている物の、目からの奥行き（m）。空なら kSsrSkyDepth。
// inverseProjectionView：射影×ビューの逆（行）。viewDepthRow：射影×ビューの 4 行目（位置との内積が奥行き）。
float ssrViewDepthAt(Texture2D<float> depthTexture, float4 inverseProjectionView[4], float4 viewDepthRow, int2 pixel, uint2 depthSize)
{
    int2 clamped = clamp(pixel, int2(0, 0), int2(depthSize) - 1);
    float depth = depthTexture.Load(int3(clamped, 0));
    float viewDepth = kSsrSkyDepth;
    if (depth < kFarDepth)
    {
        float2 uv = (clamped + 0.5) / float2(depthSize);
        viewDepth = dot(viewDepthRow, float4(worldPositionFromDepth(inverseProjectionView, uv, depth), 1.0));
    }
    return viewDepth;
}

// 半分の大きさの結果を、元の画面の画素 pixel（奥行き viewDepth）へ拡大して読む（SsaoUpsample.hlsli の upsampleOcclusion と同じ考え）。
// 近くの 4 画素を、双線形の重み × 奥行きの近さで混ぜる。4 画素とも奥行きが大きく違うときは、映り込み無し（0）にする。
float4 upsampleReflection(Texture2D reflection, Texture2D<float> depthTexture, float4 inverseProjectionView[4], float4 viewDepthRow,
                          uint2 pixel, float viewDepth)
{
    uint width, height;
    reflection.GetDimensions(width, height);
    uint depthWidth, depthHeight;
    depthTexture.GetDimensions(depthWidth, depthHeight);
    int2 last = int2(width, height) - 1;
    float2 h = pixel * kSsaoHalfStep;
    int2 base = (int2)floor(h);
    float2 f = h - base;
    float weightSum = 0.0;
    float4 sum = 0.0;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        int2 offset = int2(i & 1, i >> 1);
        int2 halfPixel = clamp(base + offset, int2(0, 0), last);
        float texelDepth = ssrViewDepthAt(depthTexture, inverseProjectionView, viewDepthRow, halfPixel * kSsrHalfStep, uint2(depthWidth, depthHeight));
        float bilinear = (offset.x ? f.x : 1.0 - f.x) * (offset.y ? f.y : 1.0 - f.y);
        float weight = bilinear * ssaoDepthWeight(texelDepth, viewDepth);
        weightSum += weight;
        sum += reflection.Load(int3(halfPixel, 0)) * weight;
    }
    return (weightSum > kSsaoMinWeight) ? sum / weightSum : float4(0.0, 0.0, 0.0, 0.0);
}

#endif
