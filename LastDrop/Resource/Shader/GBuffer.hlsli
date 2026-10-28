// G-Buffer の並び（SummaryPlan/個別仕様/GBuffer_Plan.txt 4 章）。書く側（StaticMeshGBuffer.hlsl・SkinnedMeshGBuffer.hlsl）と
// 読む側（DeferredLighting.hlsl）が、並びをここだけで知る。並びを変えるときは、ここと GBuffer.cpp の形式を一緒に直す。
//   t2 / RT0  RGBA8_SRGB : rgb 色（頂点色×材質の色×画像。リニアで書くと GPU が sRGB にして入れる）  a 予備
//   t3 / RT1  RGBA16F    : xyz 法線（ワールド・長さ 1・法線マップ済み）                          a スペキュラの鋭さ
//   t4 / RT2  RGBA8_SRGB : rgb スペキュラの色（(2+鋭さ)/2π を掛ける前。1 を超えない）            a 予備
//   t5        深度       : PostProcess の深度（位置は深度から戻す）
// 段階 3 で RT2 を「AO・粗さ・金属・発光」へ入れ替える（Graphics_Spec.txt 4-1）。
#ifndef INCLUDED_GBUFFER_HLSLI
#define INCLUDED_GBUFFER_HLSLI

#include "Lighting.hlsli"

// GameLib/src/Graphics/ManagerImpl.h の sendLightingParameters と同じ 1/(2π)。スペキュラの色に (2+鋭さ)/2π が掛けて渡される。
static const float kRcpTwoPi = 1.0 / (3.14159265 * 2.0);
static const float kSpecularNormalizeBase = 2.0;
static const float kSpare = 0.0; // 予備のチャンネルに書く値

struct GBufferOutput
{
    float4 albedo   : SV_Target0;
    float4 normal   : SV_Target1;
    float4 specular : SV_Target2;
};

// 固定パイプラインの PSConstants（b0）のスペキュラの色から、(2+鋭さ)/2π を掛ける前の材質の色へ戻す。
float3 materialSpecularColor()
{
    return gSpecularColor.xyz / ((kSpecularNormalizeBase + gSpecularColor.w) * kRcpTwoPi);
}

// materialSpecularColor の逆。G-Buffer に入れた材質の色へ、もう一度 (2+鋭さ)/2π を掛ける（lightSurface へ渡す形）。
float3 normalizedSpecularColor(float3 materialColor, float sharpness)
{
    return materialColor * ((kSpecularNormalizeBase + sharpness) * kRcpTwoPi);
}

// 1 画素ぶんの面の情報を G-Buffer の並びにする。color: 頂点色×ディフューズ色（リニア）。
// 法線マップは今の描き方（Lighting.hlsli の shadePixel）と同じ方法で掛ける。
GBufferOutput writeGBuffer(float4 color, float2 uv, float3 worldPosition, float3 worldNormal, bool isFrontFace)
{
    float3 n = normalize(worldNormal);
    n = isFrontFace ? n : -n; // 裏を描いたとき（doublesided="1" の材質）は、見ている側へ法線を向ける
    n = perturbNormal(n, worldPosition, uv);
    GBufferOutput o;
    o.albedo = float4((color * gAlbedo.Sample(gWrapSampler, uv)).rgb, kSpare);
    o.normal = float4(n, gSpecularColor.w);
    o.specular = float4(materialSpecularColor(), kSpare);
    return o;
}

// 読む側。番号は DeferredLighting.cpp と揃える。
Texture2D        gGBufferAlbedo   : register(t2);
Texture2D        gGBufferNormal   : register(t3);
Texture2D        gGBufferSpecular : register(t4);
Texture2D<float> gSceneDepth      : register(t5);

#endif
