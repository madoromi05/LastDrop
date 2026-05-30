#ifndef INCLUDED_GAMELIB_GRAPHICS_PIXELLIGHTINGPS_HLSL_H
#define INCLUDED_GAMELIB_GRAPHICS_PIXELLIGHTINGPS_HLSL_H
namespace {
const char gPixelLightingPsHlsl[] = R"HLSL(
cbuffer PSConstants : register(b0)
{
    float4 gAmbient;
    float4 gLightPosX;
    float4 gLightPosY;
    float4 gLightPosZ;
    float4 gEyePosition;
    float4 gSpecularColor;
    float4 gLightColor[4];
    float4 gEmissionColor;
};
Texture2D    gTexture : register(t0);
SamplerState gSampler : register(s0);
struct PSIn
{
    float4 pos   : SV_Position;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
    float3 wpos  : TEXCOORD1;
    float3 wnorm : TEXCOORD2;
};
float4 main(PSIn p) : SV_Target
{
    float3 ev = gEyePosition.xyz - p.wpos;
    float3 nv = normalize(p.wnorm);
    float3 rv = 2.0f * nv * dot(nv, ev) - ev;
    float4 lvX = gLightPosX - p.wpos.xxxx;
    float4 lvY = gLightPosY - p.wpos.yyyy;
    float4 lvZ = gLightPosZ - p.wpos.zzzz;
    float4 s = lvX * rv.xxxx + lvY * rv.yyyy + lvZ * rv.zzzz;
    float4 lvL2 = lvX * lvX + lvY * lvY + lvZ * lvZ + 0.0001f;
    float4 rcpLvL = rsqrt(lvL2);
    float4 rvL2 = dot(rv, rv);
    s *= rsqrt(lvL2 * rvL2);
    s = max(0, s);
    s = pow(s, gSpecularColor.w);
    s *= rcpLvL;
    float4 d = (lvX * nv.xxxx + lvY * nv.yyyy + lvZ * nv.zzzz) * rcpLvL * rcpLvL;
    d = max(0, d);
    float3 dc = gAmbient.xyz;
    dc += gLightColor[0].xyz * d.x;
    dc += gLightColor[1].xyz * d.y;
    dc += gLightColor[2].xyz * d.z;
    dc += gLightColor[3].xyz * d.w;
    float3 sc = 0;
    sc += gLightColor[0].xyz * s.x;
    sc += gLightColor[1].xyz * s.y;
    sc += gLightColor[2].xyz * s.z;
    sc += gLightColor[3].xyz * s.w;
    float4 c = p.color * gTexture.Sample(gSampler, p.uv);
    c.xyz *= dc;
    c.xyz += sc * gSpecularColor.xyz;
    c.xyz += gEmissionColor.xyz;
    return c;
}
)HLSL";
} // anonymous namespace
#endif
