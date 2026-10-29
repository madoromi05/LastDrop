// G-Buffer を読んで画面を作る全画面のパス（DeferredLighting。GBuffer_Plan.txt 7・8 章）。2 つの使い方がある。
//   表示が Final（0）：ライティング。PBR の式（Pbr.hlsli。PbrNightLighting_Plan.txt 4 章）で、シーンの色（HDR・リニア）へ描く。
//                    何も描かれていない画素には空を描く（LightList.hlsli の skyColor。7 章）
//   それ以外       ：G-Buffer の中身の表示。ToneMap を通さず、PostProcess の後に画面（バックバッファ・ガンマの色）へ直接写す
// 霧はここでは掛けない（次の霧のパス Fog.hlsl。NightFinish_Plan.txt 5-1）。霧の量の表示（Fog）だけ同じ式を読む。
#include "GBuffer.hlsli"
#include "Pbr.hlsli"
#include "DeferredConstants.hlsli"
#include "Fog.hlsli"

// DeferredLighting.h の GBufferView と同じ番号。
static const uint kViewFinal     = 0;
static const uint kViewAlbedo    = 1;
static const uint kViewNormal    = 2;
static const uint kViewRoughness = 3;
static const uint kViewMetallic  = 4;
static const uint kViewEmission  = 5;
static const uint kViewDepth     = 6;
static const uint kViewPosition  = 7;
static const uint kViewCascade   = 8;
static const uint kViewShadowMaps = 9;
static const uint kViewFog       = 10;

static const float3 kNothingColor = float3(0.0, 0.0, 0.0);

// ── 影の表示（Shadow_Plan.txt 9 章。ライトの一覧と影のマップを付けて描く。GameScene::draw）──
// 月の影の枚ごとの色（1 枚目 赤・2 枚目 緑・3 枚目 青）。下の絵の明るさに掛けて、どこが何枚目かを見る。
static const float3 kCascadeColors[3] = { float3(1.0, 0.25, 0.25), float3(0.25, 1.0, 0.25), float3(0.25, 0.5, 1.0) };
static const float kCascadeBaseBrightness = 0.3; // 下の絵が真っ黒でも枚の色が見えるよう、明るさの下限
static const float kHalfScreen = 0.5;
// 懐中電灯の影のマップは透視なので、深度がほとんど 1 の近くに集まる。累乗で広げて、近い物と遠い物の差を見えるようにする。
static const float kPerspectiveDepthContrast = 32.0;

// 画面を 4 つに分けて影のマップを写す（左上 懐中電灯・右上 月の 1 枚目・左下 2 枚目・右下 3 枚目）。付いていない影のマップは黒。
float3 shadowMapsView(float2 uv)
{
    uint column = (uv.x >= kHalfScreen) ? 1 : 0;
    uint row = (uv.y >= kHalfScreen) ? 1 : 0;
    float2 local = frac(uv / kHalfScreen);
    uint index = row * 2 + column;
    float depth;
    if (index == 0) { depth = pow(saturate(gShadowMap0.SampleLevel(gClampSampler, local, 0)), kPerspectiveDepthContrast); }
    else if (index == 1) { depth = gShadowMap1.SampleLevel(gClampSampler, local, 0); }
    else if (index == 2) { depth = gShadowMap2.SampleLevel(gClampSampler, local, 0); }
    else { depth = gShadowMap3.SampleLevel(gClampSampler, local, 0); }
    return depth;
}

// 月の影のどの枚かを色で塗る（範囲の外は下の絵のまま）。奥行きの測り方は cascadedShadowVisibility と同じ。
float3 cascadeView(float3 world, float3 albedo)
{
    float brightness = max(dot(linearToSrgb(albedo), float3(0.299, 0.587, 0.114)), kCascadeBaseBrightness);
    float depth = dot(world - gEyePosition.xyz, gCascadeView.xyz);
    float3 tint = float3(1.0, 1.0, 1.0);
    if (depth < gCascadeEnds.x) { tint = kCascadeColors[0]; }
    else if (depth < gCascadeEnds.y) { tint = kCascadeColors[1]; }
    else if (depth < gCascadeEnds.z) { tint = kCascadeColors[2]; }
    return brightness * tint;
}

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    // 写す先（画面）の描画範囲は G-Buffer と大きさ・位置がずれることがある（setViewport の範囲）ので、uv から画素を求める。
    uint width, height;
    gSceneDepth.GetDimensions(width, height);
    int3 pixel = int3(p.uv * float2(width, height), 0);
    float depth = gSceneDepth.Load(pixel);
    uint view = (uint)gViewParams.x;
    if (view == kViewShadowMaps)
    {
        return float4(shadowMapsView(p.uv), 1.0); // 画面の物とは関係なく、影のマップそのものを写す
    }
    if (depth >= kFarDepth)
    {
        if (view == kViewFinal)
        {
            // 何も描かれていない所は空。目から、この画素の一番奥（深度 1）へ向かう向きでグラデーションを引く。
            float3 skyDirection = normalize(reconstructWorldPosition(p.uv, kFarDepth) - gEyePosition.xyz);
            return float4(skyColor(skyDirection), 1.0);
        }
        return float4(kNothingColor, 1.0);
    }
    // RT2：r AO  g 粗さ  b 金属  a 発光の強さ（GBuffer.hlsli）
    float4 material = gGBufferMaterial.Load(pixel);
    float3 c = kNothingColor;
    if (view == kViewFinal)
    {
        Surface s;
        s.position = reconstructWorldPosition(p.uv, depth);
        s.normal = gGBufferNormal.Load(pixel).xyz;
        s.albedo = gGBufferAlbedo.Load(pixel).rgb;
        s.occlusion = material.r;
        s.roughness = material.g;
        s.metallic = material.b;
        c = shadeSurface(s) + s.albedo * material.a * kEmissionScale;
    }
    else if (view == kViewAlbedo)
    {
        c = linearToSrgb(gGBufferAlbedo.Load(pixel).rgb);
    }
    else if (view == kViewNormal)
    {
        c = gGBufferNormal.Load(pixel).xyz * 0.5 + 0.5; // -1〜1 を 0〜1 へ
    }
    else if (view == kViewRoughness)
    {
        c = material.g;
    }
    else if (view == kViewMetallic)
    {
        c = material.b;
    }
    else if (view == kViewEmission)
    {
        c = material.a;
    }
    else if (view == kViewDepth)
    {
        float3 world = reconstructWorldPosition(p.uv, depth);
        c = saturate(length(world - gEyePosition.xyz) / gViewParams.y);
    }
    else if (view == kViewPosition)
    {
        // 1m ごとの格子。位置の戻しが正しければ、カメラを動かしても格子は壁・地面に貼り付いたまま。
        c = frac(reconstructWorldPosition(p.uv, depth));
    }
    else if (view == kViewCascade)
    {
        c = cascadeView(reconstructWorldPosition(p.uv, depth), gGBufferAlbedo.Load(pixel).rgb);
    }
    else if (view == kViewFog)
    {
        // 霧の量（0 黒〜1 白）。空の画素は上で黒にしている（霧のパスも空には掛けない）。
        c = fogAmount(reconstructWorldPosition(p.uv, depth));
    }
    return float4(c, 1.0);
}
