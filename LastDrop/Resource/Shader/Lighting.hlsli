// 固定パイプラインの「ピクセルごとのライティング＋法線マップ」（GameLib/src/Graphics/PixelLightingNormalMapPsHlsl.h）と
// 同じ b0 の並びと、同じ法線マップの掛け方。自作シェーダが、固定パイプラインと同じ材質の値を読むために使う
// ライト・材質の値は Graphics::Manager の setLight〜 / setSpecular〜 / setNormalMapStrength で決まり、b0 に入る
// 法線マップを使わないときは、強さ（setNormalMapStrength）を 0 にすれば元の法線のまま（t1 が空でも読まれない値になる）
#ifndef INCLUDED_LIGHTING_HLSLI
#define INCLUDED_LIGHTING_HLSLI

#include "Common.hlsli"

// b0（ピクセルシェーダ側）。GameLib/src/Graphics/ManagerImpl.h の PSCBufferData と同じ並び
// 頂点シェーダ側の b0（Common.hlsli の VSConstants）とは別物（ステージごとに b0 が別にある）
cbuffer PSConstants : register(b0)
{
    float4 gAmbient;
    float4 gPsLightPosX;
    float4 gPsLightPosY;
    float4 gPsLightPosZ;
    float4 gEyePosition;
    float4 gSpecularColor;  // xyz: 色（鋭さで正規化済み）  w: 鋭さ
    float4 gPsLightColor[4];
    float4 gEmissionColor;
    float4 gMaterialParams; // x: 法線マップの強さ
};

Texture2D gAlbedo    : register(t0); // Manager::setTexture
Texture2D gNormalMap : register(t1); // Manager::setNormalMap

// 頂点タンジェント無しで、位置とUVの画面微分から接線基底を作り法線を摂動する
float3 perturbNormal(float3 normal, float3 worldPosition, float2 uv)
{
    float3 dp1 = ddx(worldPosition);
    float3 dp2 = ddy(worldPosition);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);
    float3 dp2perp = cross(dp2, normal);
    float3 dp1perp = cross(normal, dp1);
    float3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 bitangent = dp2perp * duv1.y + dp1perp * duv2.y;
    float invmax = rsqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)));
    tangent *= invmax;
    bitangent *= invmax;
    // 法線マップは XY だけ使い、Z は再構成する（青チャンネルの規約差に強い）
    float2 nxy = (gNormalMap.Sample(gWrapSampler, uv).xy * 2.0f - 1.0f) * gMaterialParams.x;
    float nz = sqrt(saturate(1.0f - dot(nxy, nxy)));
    float3 mapNormal = float3(nxy, nz);
    float3x3 tbn = float3x3(tangent, bitangent, normal);
    float3 r = normalize(mul(mapNormal, tbn));
    return any(isnan(r)) ? normal : r; // 退化（UV面積0など）のときは元の法線へ
}

#endif
