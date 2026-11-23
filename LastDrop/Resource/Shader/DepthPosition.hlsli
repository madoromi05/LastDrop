// 深度からワールドの位置を戻す式。逆行列をどの定数で持つかは使う側が決める
// （ライティング・霧は DeferredConstants.hlsli の b1、SSAO は Ssao.hlsl の b1）
#ifndef INCLUDED_DEPTH_POSITION_HLSLI
#define INCLUDED_DEPTH_POSITION_HLSLI

static const float kFarDepth = 1.0; // 何も描かれていない画素の深度（消した値）

// 画面の位置（uv）と深度から、ワールドの位置へ戻す（射影×ビューの逆を掛けて w で割る）
// inverseProjectionView：射影×ビューの逆行列（行ごと。Matrix44 の m00..m33 の順）
float3 worldPositionFromDepth(float4 inverseProjectionView[4], float2 uv, float depth)
{
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float4 world = float4(dot(inverseProjectionView[0], clip), dot(inverseProjectionView[1], clip),
                          dot(inverseProjectionView[2], clip), dot(inverseProjectionView[3], clip));
    return world.xyz / world.w;
}

#endif
