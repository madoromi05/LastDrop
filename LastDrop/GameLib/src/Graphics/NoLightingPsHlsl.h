#ifndef INCLUDED_GAMELIB_GRAPHICS_NOLIGHTINGPS_HLSL_H
#define INCLUDED_GAMELIB_GRAPHICS_NOLIGHTINGPS_HLSL_H
namespace {
const char gNoLightingPsHlsl[] = R"HLSL(
Texture2D    gTexture : register(t0);
SamplerState gSampler : register(s0);
struct PSIn
{
    float4 pos   : SV_Position;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};
float4 main(PSIn p) : SV_Target
{
    return gTexture.Sample(gSampler, p.uv) * p.color;
}
)HLSL";
} // anonymous namespace
#endif
