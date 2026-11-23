// 霧のパス（DeferredLighting::drawFog）。ライティングの直後に全画面で 1 回描く
// 深度から位置を戻して霧の量を出し、（霧の色, 霧の量）を出す。BLEND_LINEAR で、ライティングした色と霧の色を量の割合で混ぜる
// （シーンの色は読まないので、写し直しが要らない）
// 空の画素（深度 1）は霧の量 0 で何も変えない（空は地平線へ向かって霧と同じ色になる）
// 弾痕などの半透明の板はこの後に描くので、板は自分で同じ式の霧を掛ける（LitQuad.hlsl。二重には掛からない）
#include "GBuffer.hlsli"
#include "DeferredConstants.hlsli"
#include "Fog.hlsli"

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    // 描画範囲と深度の大きさがずれることがあるので、uv から画素を求める（DeferredLighting.hlsl と同じ）
    uint width, height;
    gSceneDepth.GetDimensions(width, height);
    int3 pixel = int3(p.uv * float2(width, height), 0);
    float depth = gSceneDepth.Load(pixel);
    float amount = 0.0;
    if (depth < kFarDepth)
    {
        amount = fogAmount(reconstructWorldPosition(p.uv, depth));
    }
    return float4(fogColor(), amount);
}
