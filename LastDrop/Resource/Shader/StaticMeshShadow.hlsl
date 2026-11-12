// 静的メッシュ（建物・車・置物）を影のマップへ深度だけ描く（StaticMeshPart::draw。RenderPass が Shadow の間。Shadow_Plan.txt 4 章）
// 行列は ShadowMap::begin が光の側の射影×ビューにしてあるので、頂点の計算は G-Buffer へ描くとき（StaticMeshGBuffer.hlsl）と同じ toClip だけ
// 色は書かない（描画先は深度だけ）。ピクセルシェーダは何もしない
#include "Common.hlsli"

struct ShadowVertexOutput
{
    float4 position : SV_Position;
};

ShadowVertexOutput VSMain(StandardVertex v)
{
    ShadowVertexOutput o;
    o.position = toClip(v.position);
    return o;
}

void PSMain(ShadowVertexOutput p)
{
}
