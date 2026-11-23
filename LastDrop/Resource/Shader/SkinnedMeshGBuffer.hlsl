// スキンメッシュ（キャラ）を GPU で変形して、G-Buffer へ描く（SkinningPipeline。RenderPass が GBuffer の間）
// 変形（VSMain）は Skinning.hlsli。G-Buffer の並びは GBuffer.hlsli
#include "Skinning.hlsli"
#include "GBuffer.hlsli"

GBufferOutput PSMain(PixelInput p, bool isFrontFace : SV_IsFrontFace)
{
    return writeGBuffer(p.color, p.uv, p.worldPosition, p.worldNormal, isFrontFace);
}
