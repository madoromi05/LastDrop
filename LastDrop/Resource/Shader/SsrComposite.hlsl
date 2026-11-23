// 映り込み（SSR）をシーンの色へ混ぜる全画面のパス（DeferredLighting::drawReflection）
// ライティングの環境光の鏡面は「空と地面の色 × 映り込みの強さ × AO」（Pbr.hlsli の ambientLighting）
// 画面に映っている物が見つかった所（確からしさ）だけ、その「空と地面の色」を SSR の色へ入れ替える：
//   足す量 ＝（SSR の色 × 確からしさ − 空と地面の色 × 確からしさ）× 映り込みの強さ × AO
// BLEND_ADDITIVE（アルファ 1 で足す）でシーンの色に足す。シーンの色は RGBA16F なので、負の数（空の分を引く）も足せる
// 霧の前に描く（映り込みにも霧が掛かる）
#include "GBuffer.hlsli"
#include "Pbr.hlsli"
#include "DeferredConstants.hlsli"
#include "SsrCommon.hlsli"

Texture2D gOcclusion  : register(t10); // SSAO の結果（ShaderSlots::kOcclusion）
Texture2D gReflection : register(t13); // SSR の結果（ShaderSlots::kReflection。半分の大きさ）

static const float4 kNothing = float4(0.0, 0.0, 0.0, 1.0);

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    uint width, height;
    gSceneDepth.GetDimensions(width, height);
    uint2 pixel = uint2(p.uv * float2(width, height));
    float depth = gSceneDepth.Load(int3(pixel, 0));
    float4 result = kNothing;
    if (depth < kFarDepth)
    {
        float4 material = gGBufferMaterial.Load(int3(pixel, 0));
        Surface s;
        s.position = reconstructWorldPosition(p.uv, depth);
        s.normal = normalize(gGBufferNormal.Load(int3(pixel, 0)).xyz);
        s.albedo = gGBufferAlbedo.Load(int3(pixel, 0)).rgb;
        s.roughness = material.g;
        s.metallic = material.b;
        float viewDepth = viewDepthOf(s.position);
        s.occlusion = material.r * upsampleOcclusion(gOcclusion, pixel, viewDepth);
        float4 reflection = upsampleReflection(gReflection, gSceneDepth, gInverseProjectionView, gViewDepthRow, pixel, viewDepth);
        float3 v = normalize(gEyePosition.xyz - s.position);
        float3 environment = environmentLight(reflect(-v, s.normal), s.roughness);
        float3 change = (reflection.rgb - environment * reflection.a) * environmentSpecularWeight(s, v) * s.occlusion;
        result = float4(change, 1.0);
    }
    return result;
}
