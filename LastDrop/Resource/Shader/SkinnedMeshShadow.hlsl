// スキンメッシュ（ゾンビ・自キャラ）を GPU で変形して、影のマップへ深度だけ描く（SkinningPipeline。RenderPass が Shadow の間）
// 変形（VSMain）は SkinnedMeshGBuffer.hlsl と同じ Skinning.hlsli。色は書かないので、ピクセルシェーダは何もしない
#include "Skinning.hlsli"

void PSMain(PixelInput p)
{
}
