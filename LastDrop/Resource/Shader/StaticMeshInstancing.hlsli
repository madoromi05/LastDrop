// 静的メッシュ（建物）をインスタンシングで描くときの置き場所（StaticMeshInstancing。OptimizationA_Plan.txt 8 章）。
// 1 回の描画で同じ形を何軒も描き、何軒目かを SV_InstanceID で受け取って、置き場所の行列を b1 の配列から読む。
// ゾンビの骨の行列（Skinning.hlsli）と同じ作りで、b0 のワールド行列は単位行列にしてある（gTransform = 射影×ビュー）。
#ifndef INCLUDED_STATIC_MESH_INSTANCING_HLSLI
#define INCLUDED_STATIC_MESH_INSTANCING_HLSLI

#include "Common.hlsli"

#define MAX_INSTANCES 128 // StaticMeshInstancing.h の kMaxInstances と同じ値にする

// 1 軒 = 3 行（3x4 行列の行ごとの float4）。
cbuffer InstancePlacements : register(b1)
{
    float4 gPlacementRows[MAX_INSTANCES * 3];
};

// モデルの空間の点をワールドへ置く。
float3 placePoint(float4 p, uint instance)
{
    uint row = instance * 3;
    return float3(dot(gPlacementRows[row], p), dot(gPlacementRows[row + 1], p), dot(gPlacementRows[row + 2], p));
}

// モデルの空間の向き（法線）をワールドへ回す。建物は等倍で置くので、逆行列の転置ではなく行列の 3x3 でそのまま回せる。
float3 placeVector(float3 v, uint instance)
{
    uint row = instance * 3;
    return float3(dot(gPlacementRows[row].xyz, v), dot(gPlacementRows[row + 1].xyz, v), dot(gPlacementRows[row + 2].xyz, v));
}

#endif
