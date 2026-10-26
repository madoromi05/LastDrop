#ifndef INCLUDED_GAMELIB_GRAPHICS_VERTEXLIGHTINGVS_HLSL_H
#define INCLUDED_GAMELIB_GRAPHICS_VERTEXLIGHTINGVS_HLSL_H
#include "Graphics/ColorSpaceHlsl.h"
namespace {
const char gVertexLightingVsHlsl[] = GAMELIB_COLOR_SPACE_HLSL R"HLSL(
cbuffer VSConstants : register(b0)
{
    float4 gTransform[4];
    float4 gWorldMatrix[3];
    float4 gITWorldMatrix[3];
    float4 gDiffuseColor;
    float4 gAmbientColor;
    float4 gLightPosX;
    float4 gLightPosY;
    float4 gLightPosZ;
    float4 gLightColor[4];
    float4 gColorParams; // x: 1 なら頂点色をリニアへ直す（Manager::enableLinearColor）
};
struct VSIn
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};
struct VSOut
{
    float4 pos   : SV_Position;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};
VSOut main(VSIn v)
{
    VSOut o;
    o.pos.x = dot(gTransform[0], v.position);
    o.pos.y = dot(gTransform[1], v.position);
    o.pos.z = dot(gTransform[2], v.position);
    o.pos.w = dot(gTransform[3], v.position);
    float3 wp;
    wp.x = dot(gWorldMatrix[0], v.position);
    wp.y = dot(gWorldMatrix[1], v.position);
    wp.z = dot(gWorldMatrix[2], v.position);
    float3 wn;
    wn.x = dot(gITWorldMatrix[0].xyz, v.normal);
    wn.y = dot(gITWorldMatrix[1].xyz, v.normal);
    wn.z = dot(gITWorldMatrix[2].xyz, v.normal);
    float4 lvX = gLightPosX - wp.xxxx;
    float4 lvY = gLightPosY - wp.yyyy;
    float4 lvZ = gLightPosZ - wp.zzzz;
    float4 lvL = lvX * lvX + lvY * lvY + lvZ * lvZ + 0.00001f;
    float4 d = (lvX * wn.xxxx + lvY * wn.yyyy + lvZ * wn.zzzz) / lvL;
    d = max((float4)0, d);
    float3 c = gAmbientColor.xyz;
    c += gLightColor[0].xyz * d.x;
    c += gLightColor[1].xyz * d.y;
    c += gLightColor[2].xyz * d.z;
    c += gLightColor[3].xyz * d.w;
    o.color.xyz = c * vertexColorToScene(v.color, gColorParams).xyz * gDiffuseColor.xyz;
    o.color.w   = v.color.w * gDiffuseColor.w;
    o.uv = v.uv;
    return o;
}
)HLSL";
} // anonymous namespace
#endif
