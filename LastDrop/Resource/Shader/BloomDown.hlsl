// ブルームの縮小（Bloom::apply）。1 つ大きい段を、半分の大きさの段へ縮めながらぼかす
#include "Common.hlsli"

Texture2D gSource : register(t2); // Bloom.cpp の kSourceSlot（1 つ大きい段）

// 読む所のずらし（読む元の画素単位）。書き先の 1 画素は読む元の 2×2 画素の真ん中にあたるので、
// 斜めに 1 画素ずつずらした 4 か所は、どれも 2×2 画素の真ん中になる。線形補間で読めば 4×4 画素の平均になる
static const int kTapCount = 4;
static const float2 kTapOffsets[kTapCount] =
{
    float2(-1.0, -1.0), float2(1.0, -1.0), float2(-1.0, 1.0), float2(1.0, 1.0)
};

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
        sum += gSource.Sample(gClampSampler, v.uv + kTapOffsets[i] * texel).rgb;
    }
    return float4(sum / kTapCount, 1.0);
}
