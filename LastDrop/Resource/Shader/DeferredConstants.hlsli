// 深度から位置を戻す全画面のパス（DeferredLighting.hlsl・Fog.hlsl）の共通の定数（b1）
// どちらも DeferredLighting.cpp が DeferredConstants に書いて付ける（GBuffer_Plan.txt 7 章・NightFinish_Plan.txt 5-1）
#ifndef INCLUDED_DEFERRED_CONSTANTS_HLSLI
#define INCLUDED_DEFERRED_CONSTANTS_HLSLI

#include "DepthPosition.hlsli"

// DeferredLighting.cpp の DeferredConstants と同じ並び
cbuffer DeferredConstants : register(b1)
{
    float4 gInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gViewParams;               // x: 表示する物（GBufferView の番号）  y: 深度の表示で白になる距離（m）  z: 表示の Hi-Z で見る段  w: 使わない
    float4 gViewDepthRow;             // 射影×ビューの 4 行目。位置との内積が目からの奥行き（m。SSAO の g と同じ物差し）
};

// ワールドの位置の、目からの奥行き（SSAO の結果を拡大するとき、SsaoUpsample.hlsli の upsampleOcclusion に渡す）
float viewDepthOf(float3 world)
{
    return dot(gViewDepthRow, float4(world, 1.0));
}

// 画面の位置（uv）と深度から、ワールドの位置へ戻す（式は DepthPosition.hlsli。SSAO と共有）
float3 reconstructWorldPosition(float2 uv, float depth)
{
    return worldPositionFromDepth(gInverseProjectionView, uv, depth);
}

#endif
