#ifndef INCLUDED_GAMELIB_GRAPHICS_NOLIGHTINGVS_HLSL_H
#define INCLUDED_GAMELIB_GRAPHICS_NOLIGHTINGVS_HLSL_H
#include "Graphics/ColorSpaceHlsl.h"
namespace {
const char gNoLightingVsHlsl[] = GAMELIB_COLOR_SPACE_HLSL R"HLSL(
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
    o.color = vertexColorToScene(v.color, gColorParams) * gDiffuseColor;
    o.uv = v.uv;
    return o;
}
)HLSL";
} // anonymous namespace
#endif
