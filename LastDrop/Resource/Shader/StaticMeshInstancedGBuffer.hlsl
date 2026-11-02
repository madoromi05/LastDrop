// 静的メッシュ（建物）をインスタンシングでまとめて G-Buffer へ描く（StaticMeshInstancing。RenderPass が GBuffer の間。OptimizationA_Plan.txt 8 章）。
// 1 軒ずつ描く StaticMeshGBuffer.hlsl との違いは、置き場所を b0 のワールド行列ではなく b1 の配列から読むことだけ。
// G-Buffer へ書く中身は同じ writeGBuffer（GBuffer.hlsli）を使う。
#include "GBuffer.hlsli"
#include "StaticMeshInstancing.hlsli"

struct PixelInput
{
    float4 position      : SV_Position;
    float4 color         : COLOR0;
    float2 uv            : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
    float3 worldNormal   : TEXCOORD2;
};

PixelInput VSMain(StandardVertex v, uint instance : SV_InstanceID)
{
    PixelInput o;
    o.worldPosition = placePoint(v.position, instance);
    o.position = toClip(float4(o.worldPosition, 1.0)); // ワールド行列は単位行列なので、gTransform は射影×ビュー
    o.worldNormal = placeVector(v.normal, instance);
    o.color = vertexColor(v.color) * gDiffuseColor;
    o.uv = v.uv;
    return o;
}

GBufferOutput PSMain(PixelInput p, bool isFrontFace : SV_IsFrontFace)
{
    return writeGBuffer(p.color, p.uv, p.worldPosition, p.worldNormal, isFrontFace);
}
