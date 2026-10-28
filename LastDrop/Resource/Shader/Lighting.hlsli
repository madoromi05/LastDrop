// 固定パイプラインの「ピクセルごとのライティング＋法線マップ」（GameLib/src/Graphics/PixelLightingNormalMapPsHlsl.h）と
// 同じ計算。自作シェーダで描くものも、固定パイプラインで描くものと同じ見た目にするために使う。
// ライト・材質の値は Graphics::Manager の setLight〜 / setSpecular〜 / setNormalMapStrength で決まり、b0 に入る。
// 法線マップを使わないときは、強さ（setNormalMapStrength）を 0 にすれば元の法線のまま（t1 が空でも読まれない値になる）。
#ifndef INCLUDED_LIGHTING_HLSLI
#define INCLUDED_LIGHTING_HLSLI

#include "Common.hlsli"

// b0（ピクセルシェーダ側）。GameLib/src/Graphics/ManagerImpl.h の PSCBufferData と同じ並び。
// 頂点シェーダ側の b0（Common.hlsli の VSConstants）とは別物（ステージごとに b0 が別にある）。
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

// 頂点タンジェント無しで、位置とUVの画面微分から接線基底を作り法線を摂動する。
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
    // 法線マップは XY だけ使い、Z は再構成する（青チャンネルの規約差に強い）。
    float2 nxy = (gNormalMap.Sample(gWrapSampler, uv).xy * 2.0f - 1.0f) * gMaterialParams.x;
    float nz = sqrt(saturate(1.0f - dot(nxy, nxy)));
    float3 mapNormal = float3(nxy, nz);
    float3x3 tbn = float3x3(tangent, bitangent, normal);
    float3 r = normalize(mul(mapNormal, tbn));
    return any(isnan(r)) ? normal : r; // 退化（UV面積0など）のときは元の法線へ
}

// 1 つの面を 4 灯のディフューズ・スペキュラ＋環境光で照らした色（発光は含まない）。今の描き方（shadePixel）と
// ディファードのライティングのパス（DeferredLighting.hlsl）が同じこの関数を呼ぶ（GBuffer_Plan.txt 7 章。式を 2 回書かない）。
// albedo: 頂点色×ディフューズ色×画像（リニア）  nv: 法線（長さ 1・法線マップ済み）
// specularColor: (2+鋭さ)/2π を掛けた後のスペキュラの色（b0 の gSpecularColor.xyz と同じ形）  sharpness: 鋭さ
float3 lightSurface(float3 albedo, float3 nv, float3 worldPosition, float3 specularColor, float sharpness)
{
    float3 ev = gEyePosition.xyz - worldPosition;
    float3 rv = 2.0f * nv * dot(nv, ev) - ev;
    float4 lvX = gPsLightPosX - worldPosition.xxxx;
    float4 lvY = gPsLightPosY - worldPosition.yyyy;
    float4 lvZ = gPsLightPosZ - worldPosition.zzzz;
    float4 s = lvX * rv.xxxx + lvY * rv.yyyy + lvZ * rv.zzzz;
    float4 lvL2 = lvX * lvX + lvY * lvY + lvZ * lvZ + 0.0001f;
    float4 rcpLvL = rsqrt(lvL2);
    float4 rvL2 = dot(rv, rv);
    s *= rsqrt(lvL2 * rvL2);
    s = max(0, s);
    s = pow(s, sharpness);
    s *= rcpLvL;
    float4 d = (lvX * nv.xxxx + lvY * nv.yyyy + lvZ * nv.zzzz) * rcpLvL * rcpLvL;
    d = max(0, d);
    float3 dc = gAmbient.xyz;
    dc += gPsLightColor[0].xyz * d.x;
    dc += gPsLightColor[1].xyz * d.y;
    dc += gPsLightColor[2].xyz * d.z;
    dc += gPsLightColor[3].xyz * d.w;
    float3 sc = 0;
    sc += gPsLightColor[0].xyz * s.x;
    sc += gPsLightColor[1].xyz * s.y;
    sc += gPsLightColor[2].xyz * s.z;
    sc += gPsLightColor[3].xyz * s.w;
    return albedo * dc + sc * specularColor;
}

// 4灯のディフューズ・スペキュラ＋環境光＋発光で、1画素の色を求める。color: 頂点色×ディフューズ色。
float4 shadePixel(float4 color, float2 uv, float3 worldPosition, float3 worldNormal)
{
    float3 nv = normalize(worldNormal);
    nv = perturbNormal(nv, worldPosition, uv);
    float4 c = color * gAlbedo.Sample(gWrapSampler, uv);
    c.xyz = lightSurface(c.xyz, nv, worldPosition, gSpecularColor.xyz, gSpecularColor.w);
    c.xyz += gEmissionColor.xyz;
    return c;
}

#endif
