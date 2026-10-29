// 深度から位置を戻す全画面のパス（DeferredLighting.hlsl・Fog.hlsl）の共通の定数（b1）。
// どちらも DeferredLighting.cpp が DeferredConstants に書いて付ける（GBuffer_Plan.txt 7 章・NightFinish_Plan.txt 5-1）。
#ifndef INCLUDED_DEFERRED_CONSTANTS_HLSLI
#define INCLUDED_DEFERRED_CONSTANTS_HLSLI

// DeferredLighting.cpp の DeferredConstants と同じ並び。
cbuffer DeferredConstants : register(b1)
{
    float4 gInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gViewParams;               // x: 表示する物（GBufferView の番号）  y: 深度の表示で白になる距離（m）  zw: 使わない
};

static const float kFarDepth = 1.0; // 何も描かれていない画素の深度（消した値）

// 画面の位置（uv）と深度から、ワールドの位置へ戻す（射影×ビューの逆を掛けて w で割る）。
float3 reconstructWorldPosition(float2 uv, float depth)
{
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float4 world = float4(dot(gInverseProjectionView[0], clip), dot(gInverseProjectionView[1], clip),
                          dot(gInverseProjectionView[2], clip), dot(gInverseProjectionView[3], clip));
    return world.xyz / world.w;
}

#endif
