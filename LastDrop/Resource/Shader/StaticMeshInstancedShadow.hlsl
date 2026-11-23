// 静的メッシュ（建物）をインスタンシングでまとめて影のマップへ深度だけ描く（StaticMeshInstancing。RenderPass が Shadow の間）
// 1 軒ずつ描く StaticMeshShadow.hlsl との違いは、置き場所を b1 の配列から読むことだけ。色は書かないので、ピクセルシェーダは何もしない
#include "StaticMeshInstancing.hlsli"

struct ShadowVertexOutput
{
    float4 position : SV_Position;
};

ShadowVertexOutput VSMain(StandardVertex v, uint instance : SV_InstanceID)
{
    ShadowVertexOutput o;
    o.position = toClip(float4(placePoint(v.position, instance), 1.0)); // ワールド行列は単位行列なので、gTransform は光の側の射影×ビュー
    return o;
}

void PSMain(ShadowVertexOutput p)
{
}
