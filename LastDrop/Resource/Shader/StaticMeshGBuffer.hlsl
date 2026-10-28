// 静的メッシュ（建物・地面・車・置物・弾）を G-Buffer へ描く（StaticMeshPart::draw。RenderPass が GBuffer の間）。
// 頂点の計算は固定パイプラインの「ピクセルごとのライティング」（GameLib/src/Graphics/PixelLightingVsHlsl.h）と同じ。
// 材質の値は今どおり MeshMaterial::apply が Graphics::Manager へ渡し、b0 から読む。G-Buffer の並びは GBuffer.hlsli。
#include "GBuffer.hlsli"

struct PixelInput
{
    float4 position      : SV_Position;
    float4 color         : COLOR0;
    float2 uv            : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
    float3 worldNormal   : TEXCOORD2;
};

PixelInput VSMain(StandardVertex v)
{
    PixelInput o;
    o.position = toClip(v.position);
    o.worldPosition = toWorld(v.position);
    o.worldNormal = normalToWorld(v.normal);
    o.color = vertexColor(v.color) * gDiffuseColor;
    o.uv = v.uv;
    return o;
}

GBufferOutput PSMain(PixelInput p, bool isFrontFace : SV_IsFrontFace)
{
    return writeGBuffer(p.color, p.uv, p.worldPosition, p.worldNormal, isFrontFace);
}
