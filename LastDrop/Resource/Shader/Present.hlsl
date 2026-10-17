// 中間の描画先に描いたシーンを、画面へ写す（PostProcess::endScene）。
// 明るい所のにじみ（ブルーム。Bloom::apply が作った絵）を足してから、周辺を少し暗くする（ビネット）。
// トーンマップ・カラーグレーディングなどはここへ足していく（Graphics_Spec.txt 2 章の (10)）。
#include "Common.hlsli"

Texture2D gSceneColor : register(t2); // PostProcess.cpp の kSceneColorSlot
Texture2D gBloom      : register(t3); // PostProcess.cpp の kBloomSlot（画面の 1/2 の大きさ）

cbuffer PresentConstants : register(b1) // PostProcess.cpp の PresentConstants と同じ並び
{
    float gVignetteStrength; // 角での暗くなり具合（0で無し、1で真っ黒）
    float gVignetteInner;    // 暗くなり始める距離（画面中心0〜上下左右の端1）
    float gVignetteOuter;    // 一番暗くなる距離（角は約1.41）
    float gBloomIntensity;   // ブルームを足す強さ
};

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex v) : SV_Target
{
    float3 color = gSceneColor.Sample(gClampSampler, v.uv).rgb;
    // 1 を超えた所は、この後バックバッファへ書くときに 1 で切られる（トーンマップはまだ無い。Bloom_Plan.txt 6 章）。
    color += gBloom.Sample(gClampSampler, v.uv).rgb * gBloomIntensity;
    float distance = length(v.uv - 0.5) * 2.0;
    float darken = gVignetteStrength * smoothstep(gVignetteInner, gVignetteOuter, distance);
    return float4(color * (1.0 - darken), 1.0);
}
