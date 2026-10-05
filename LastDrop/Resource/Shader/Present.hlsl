// 中間の描画先に描いたシーンを、画面へ写す（PostProcess::endScene）。
// 今は描画基盤の動作確認のため、周辺を少し暗くする（ビネット）だけ。
// トーンマップ・カラーグレーディングなどはここへ足していく。
#include "Common.hlsli"

Texture2D gSceneColor : register(t2); // PostProcess.cpp の kSceneColorSlot

cbuffer PresentConstants : register(b1) // PostProcess.cpp の PresentConstants と同じ並び
{
    float gVignetteStrength; // 角での暗くなり具合（0で無し、1で真っ黒）
    float gVignetteInner;    // 暗くなり始める距離（画面中心0〜上下左右の端1）
    float gVignetteOuter;    // 一番暗くなる距離（角は約1.41）
    float gPadding;
};

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex v) : SV_Target
{
    float3 color = gSceneColor.Sample(gClampSampler, v.uv).rgb;
    float distance = length(v.uv - 0.5) * 2.0;
    float darken = gVignetteStrength * smoothstep(gVignetteInner, gVignetteOuter, distance);
    return float4(color * (1.0 - darken), 1.0);
}
