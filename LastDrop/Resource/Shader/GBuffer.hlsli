// G-Buffer の並び（SummaryPlan/個別仕様/GBuffer_Plan.txt 4 章・PbrNightLighting_Plan.txt 5 章）。書く側（StaticMeshGBuffer.hlsl・
// SkinnedMeshGBuffer.hlsl）と読む側（DeferredLighting.hlsl）が、並びをここだけで知る。並びを変えるときは、ここと GBuffer.cpp の形式を一緒に直す。
//   t2 / RT0  RGBA8_SRGB : rgb 色（頂点色×材質の色×画像。リニアで書くと GPU が sRGB にして入れる）  a 予備
//   t3 / RT1  RGBA16F    : xyz 法線（ワールド・長さ 1・法線マップ済み）                          a 予備
//   t4 / RT2  RGBA8      : r AO（今は 1）  g 粗さ  b 金属  a 発光の強さ（0〜1。色は「色 × 強さ × kEmissionScale」）
//   t5        深度       : PostProcess の深度（位置は深度から戻す）
#ifndef INCLUDED_GBUFFER_HLSLI
#define INCLUDED_GBUFFER_HLSLI

#include "Lighting.hlsli"

static const float kSpare = 0.0;     // 予備のチャンネルに書く値
static const float kNoOcclusion = 1.0; // AO（隙間の暗がり）。段階 6 の SSAO まで、どの面も遮られていない
// 発光の強さ 1 のときの明るさ（色に掛ける）。HDR なので 1 を超えてよい（ブルームで光る）。画面で見て決めた値ではない初期値。
static const float kEmissionScale = 8.0;

// b3：次に描く物の PBR の材質値（MeshMaterial::apply → RenderPass::setSurface。GBuffer.cpp の kSurfaceSlot）。
// b1 はキャラの骨（Skinning.hlsli）、b2 は段階 3-b のライトの一覧に使うので空けておく。
cbuffer GBufferMaterial : register(b3)
{
    float4 gSurface; // x: 粗さ  y: 金属  z: 発光の強さ  w: 使わない
};

struct GBufferOutput
{
    float4 albedo   : SV_Target0;
    float4 normal   : SV_Target1;
    float4 material : SV_Target2;
};

// 1 画素ぶんの面の情報を G-Buffer の並びにする。color: 頂点色×ディフューズ色（リニア）。
// 法線マップは今の描き方（Lighting.hlsli の shadePixel）と同じ方法で掛ける。
GBufferOutput writeGBuffer(float4 color, float2 uv, float3 worldPosition, float3 worldNormal, bool isFrontFace)
{
    float3 n = normalize(worldNormal);
    n = isFrontFace ? n : -n; // 裏を描いたとき（doublesided="1" の材質）は、見ている側へ法線を向ける
    n = perturbNormal(n, worldPosition, uv);
    GBufferOutput o;
    o.albedo = float4((color * gAlbedo.Sample(gWrapSampler, uv)).rgb, kSpare);
    o.normal = float4(n, kSpare);
    o.material = float4(kNoOcclusion, saturate(gSurface.xyz));
    return o;
}

// 読む側。番号は DeferredLighting.cpp と揃える。
Texture2D        gGBufferAlbedo   : register(t2);
Texture2D        gGBufferNormal   : register(t3);
Texture2D        gGBufferMaterial : register(t4);
Texture2D<float> gSceneDepth      : register(t5);

#endif
