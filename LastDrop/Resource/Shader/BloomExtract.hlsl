// ブルームの最初の処理（Bloom::apply）。シーンの色から、明るさがしきい値を超えた分だけを取り出し、
// 画面の 1/2 の大きさの描画先へ書く。
#include "Common.hlsli"

Texture2D gSource : register(t2); // Bloom.cpp の kSourceSlot（シーンの色）

cbuffer BloomConstants : register(b1) // Bloom.cpp の BloomConstants と同じ並び
{
    float  gThreshold; // 明るさがこれを超えた分だけ取り出す
    float3 gPadding;
};

// 真っ黒な画素で 0 で割らないための下限。どのみち取り出す量は 0 になる。
static const float kMinBrightness = 1e-4;

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex v) : SV_Target
{
    // 書き先は読む元の半分の大きさなので、uv は読む元の 2×2 画素の真ん中に来る。線形補間で 1 回読めば 4 画素の平均になる。
    float3 color = gSource.Sample(gClampSampler, v.uv).rgb;
    float brightness = max(color.r, max(color.g, color.b));
    // 色合い（r, g, b の比）を保ったまま、超えた分の割合だけを残す。
    float excess = max(brightness - gThreshold, 0.0);
    return float4(color * (excess / max(brightness, kMinBrightness)), 1.0);
}
