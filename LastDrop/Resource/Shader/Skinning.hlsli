// スキンメッシュ（キャラ）を GPU で変形する頂点シェーダ（SkinningPipeline）。色を出す SkinnedMesh.hlsl と、
// G-Buffer へ描く SkinnedMeshGBuffer.hlsl が同じ VSMain を使う（ピクセルシェーダだけが違う）。
// 頂点は1体ぶんを全キャラで共有し、骨の行列（置き場所×ポーズ×bindInverse）だけを体ごとに b1 で渡す。
// 変形の式は以前の CPU スキニング（SkinnedMeshData::deform）と同じ：位置・法線を4本の骨で重み付きに合成する。
#ifndef INCLUDED_SKINNING_HLSLI
#define INCLUDED_SKINNING_HLSLI

#include "Common.hlsli"

#define MAX_BONES 128 // SkinningPipeline.h の kMaxBones と同じ値にする

// 骨1本 = 3行（3x4 行列の行ごとの float4）。
cbuffer BonePalette : register(b1)
{
    float4 gBoneRows[MAX_BONES * 3];
};

struct SkinnedVertex
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
    uint4  bones    : BLENDINDICES;
    float4 weights  : BLENDWEIGHT;
};

struct PixelInput
{
    float4 position      : SV_Position;
    float4 color         : COLOR0;
    float2 uv            : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
    float3 worldNormal   : TEXCOORD2;
};

float3 skinPoint(float4 p, uint bone)
{
    uint row = bone * 3;
    return float3(dot(gBoneRows[row], p), dot(gBoneRows[row + 1], p), dot(gBoneRows[row + 2], p));
}

float3 skinVector(float3 v, uint bone)
{
    uint row = bone * 3;
    return float3(dot(gBoneRows[row].xyz, v), dot(gBoneRows[row + 1].xyz, v), dot(gBoneRows[row + 2].xyz, v));
}

PixelInput VSMain(SkinnedVertex v)
{
    float4 restPosition = float4(v.position.xyz, 1.0);
    float3 position = 0;
    float3 normal = 0;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        position += skinPoint(restPosition, v.bones[i]) * v.weights[i];
        normal += skinVector(v.normal, v.bones[i]) * v.weights[i];
    }
    PixelInput o;
    // 骨の行列に置き場所まで入っているので、ワールド行列は単位行列（gTransform = 射影×ビュー）。
    o.position = toClip(float4(position, 1.0));
    o.worldPosition = position;
    o.worldNormal = normal;
    o.color = vertexColor(v.color) * gDiffuseColor;
    o.uv = v.uv;
    return o;
}

#endif
