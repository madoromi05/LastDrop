// スキンメッシュ（キャラ）を GPU で変形して、その場で色を出して描く（SkinningPipeline。今の描き方＝フォワード）
// 変形（VSMain）は Skinning.hlsli。G-Buffer へ描くときは SkinnedMeshGBuffer.hlsl を使う
#include "Skinning.hlsli"
#include "Lighting.hlsli"

float4 PSMain(PixelInput p) : SV_Target
{
    return shadePixel(p.color, p.uv, p.worldPosition, p.worldNormal);
}
