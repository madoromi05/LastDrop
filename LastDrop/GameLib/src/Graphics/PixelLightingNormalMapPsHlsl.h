#ifndef INCLUDED_GAMELIB_GRAPHICS_PIXELLIGHTINGNORMALMAPPS_HLSL_H
#define INCLUDED_GAMELIB_GRAPHICS_PIXELLIGHTINGNORMALMAPPS_HLSL_H
namespace {
const char gPixelLightingNormalMapPsHlsl[] = R"HLSL(
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
Texture2D    gTexture   : register(t0);
Texture2D    gNormalMap : register(t1);
SamplerState gSampler   : register(s0);
struct PSIn
{
    float4 pos   : SV_Position;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
    float3 wpos  : TEXCOORD1;
    float3 wnorm : TEXCOORD2;
};
// 頂点タンジェント無しで、位置とUVの画面微分から接線基底を作り法線を摂動する。
float3 perturbNormal( float3 N, float3 wpos, float2 uv )
{
    float3 dp1 = ddx( wpos );
    float3 dp2 = ddy( wpos );
    float2 duv1 = ddx( uv );
    float2 duv2 = ddy( uv );
    float3 dp2perp = cross( dp2, N );
    float3 dp1perp = cross( N, dp1 );
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    float invmax = rsqrt( max( dot( T, T ), dot( B, B ) ) );
    T *= invmax; B *= invmax;
    // 法線マップは XY だけ使い、Z は再構成する（青チャンネルの規約差に強い）。
    // 末尾の係数が凹凸の強さ。0で無効、1で標準。
    float2 nxy = ( gNormalMap.Sample( gSampler, uv ).xy * 2.0f - 1.0f ) * 0.3f;
    // nxy.y = -nxy.y; // 凹凸が反転して見える場合はこの行を有効化（OpenGL式対策）
    float nz = sqrt( saturate( 1.0f - dot( nxy, nxy ) ) );
    float3 mapN = float3( nxy, nz );
    float3x3 TBN = float3x3( T, B, N );
    float3 r = normalize( mul( mapN, TBN ) );
    return any( isnan( r ) ) ? N : r; // 退化(UV面積0など)時は幾何法線へ
}
float4 main(PSIn p) : SV_Target
{
    float3 ev = gEyePosition.xyz - p.wpos;
    float3 nv = normalize( p.wnorm );
    nv = perturbNormal( nv, p.wpos, p.uv ); // 法線マップで法線を上書き
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
