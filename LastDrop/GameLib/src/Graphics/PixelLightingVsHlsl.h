#ifndef INCLUDED_GAMELIB_GRAPHICS_PIXELLIGHTINGVS_HLSL_H
#define INCLUDED_GAMELIB_GRAPHICS_PIXELLIGHTINGVS_HLSL_H
namespace {
const char gPixelLightingVsHlsl[] = R"HLSL(
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
    float3 wpos  : TEXCOORD1;
    float3 wnorm : TEXCOORD2;
};
VSOut main(VSIn v)
{
    VSOut o;
    o.pos.x = dot(gTransform[0], v.position);
    o.pos.y = dot(gTransform[1], v.position);
    o.pos.z = dot(gTransform[2], v.position);
    o.pos.w = dot(gTransform[3], v.position);
    o.wpos.x = dot(gWorldMatrix[0], v.position);
    o.wpos.y = dot(gWorldMatrix[1], v.position);
    o.wpos.z = dot(gWorldMatrix[2], v.position);
    o.wnorm.x = dot(gITWorldMatrix[0].xyz, v.normal);
    o.wnorm.y = dot(gITWorldMatrix[1].xyz, v.normal);
    o.wnorm.z = dot(gITWorldMatrix[2].xyz, v.normal);
    o.color = v.color.zyxw * gDiffuseColor;
    o.uv = v.uv;
    return o;
}
)HLSL";
} // anonymous namespace
#endif
