// ブルームの足し戻し（Bloom::apply）。1 つ小さい段をぼかしながら広げ、1 つ大きい段へ加算で描く
#include "Common.hlsli"

Texture2D gSource : register(t2); // Bloom.cpp の kSourceSlot（1 つ小さい段）

// 3×3 のテント（中心 4・辺 2・角 1）。xy が読む所のずらし（読む元の画素単位）、z が重み
// 小さい段の 1 画素が大きい段では 2×2 画素に広がるので、そのままだと四角いブロックが見える。テントでぼかして消す
static const int kTapCount = 9;
static const float3 kTaps[kTapCount] =
{
    float3(-1.0, -1.0, 1.0), float3(0.0, -1.0, 2.0), float3(1.0, -1.0, 1.0),
    float3(-1.0,  0.0, 2.0), float3(0.0,  0.0, 4.0), float3(1.0,  0.0, 2.0),
    float3(-1.0,  1.0, 1.0), float3(0.0,  1.0, 2.0), float3(1.0,  1.0, 1.0)
};
static const float kTotalWeight = 16.0; // kTaps の重みの合計

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex v) : SV_Target
{
    float2 sourceSize;
    gSource.GetDimensions(sourceSize.x, sourceSize.y);
    float2 texel = 1.0 / sourceSize;

    float3 sum = 0.0;
    [unroll]
    for (int i = 0; i < kTapCount; ++i)
    {
        sum += gSource.Sample(gClampSampler, v.uv + kTaps[i].xy * texel).rgb * kTaps[i].z;
    }
    // α は 1 で出す（BLEND_ADDITIVE は「元の色 ＋ 描く色 × α」）
    return float4(sum / kTotalWeight, 1.0);
}
