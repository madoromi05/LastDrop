// 自分で光る半透明の板（マズルフラッシュ。LitQuadBatch の MODE_GLOW）
// 色は固定パイプラインのライト無し（GameLib の NoLighting）と同じ「絵 × 頂点の色 × 材質の色（明るさの倍率）」
// 後ろに足す（BLEND_ADDITIVE。絵の色 × アルファ）ので、壁・地面に刺さった所ではアルファを下げて足す量を減らす（SoftQuad.hlsli）
// 光るのでライトの一覧も霧も読まない（目の前で光る物で、霧はほぼ 0）
#include "Lighting.hlsli"
#include "SoftQuad.hlsli"

struct PixelInput
{
    float4 position      : SV_Position;
    float4 color         : COLOR0;
    float2 uv            : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
};

PixelInput VSMain(StandardVertex v)
{
    PixelInput o;
    o.position = toClip(v.position);
    o.worldPosition = toWorld(v.position);
    o.color = vertexColor(v.color) * gDiffuseColor;
    o.uv = v.uv;
    return o;
}

float4 PSMain(PixelInput p) : SV_Target
{
    float4 c = gAlbedo.Sample(gWrapSampler, p.uv) * p.color;
    c.a *= softFade(p.position, p.worldPosition);
    return c;
}
