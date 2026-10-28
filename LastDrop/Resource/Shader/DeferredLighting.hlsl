// G-Buffer を読んで画面を作る全画面のパス（DeferredLighting。GBuffer_Plan.txt 7・8 章）。2 つの使い方がある。
//   表示が Final（0）：ライティング。今の描き方と同じ式（Lighting.hlsli の lightSurface）で、シーンの色（HDR・リニア）へ描く
//   それ以外       ：G-Buffer の中身の表示。ToneMap を通さず、PostProcess の後に画面（バックバッファ・ガンマの色）へ直接写す
#include "GBuffer.hlsli"

// DeferredLighting.cpp の DeferredConstants と同じ並び。
cbuffer DeferredConstants : register(b1)
{
    float4 gInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gViewParams;               // x: 表示する物（GBufferView の番号）  y: 深度の表示で白になる距離（m）  z: 鋭さの表示で白になる値
};

// DeferredLighting.h の GBufferView と同じ番号。
static const uint kViewFinal     = 0;
static const uint kViewAlbedo    = 1;
static const uint kViewNormal    = 2;
static const uint kViewSpecular  = 3;
static const uint kViewSharpness = 4;
static const uint kViewDepth     = 5;
static const uint kViewPosition  = 6;

static const float kFarDepth = 1.0;              // 何も描かれていない画素の深度（消した値）
static const float3 kNothingColor = float3(0.0, 0.0, 0.0);

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

// 画面の位置（uv）と深度から、ワールドの位置へ戻す（射影×ビューの逆を掛けて w で割る）。
float3 reconstructWorldPosition(float2 uv, float depth)
{
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float4 world = float4(dot(gInverseProjectionView[0], clip), dot(gInverseProjectionView[1], clip),
                          dot(gInverseProjectionView[2], clip), dot(gInverseProjectionView[3], clip));
    return world.xyz / world.w;
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    // 写す先（画面）の描画範囲は G-Buffer と大きさ・位置がずれることがある（setViewport の範囲）ので、uv から画素を求める。
    uint width, height;
    gSceneDepth.GetDimensions(width, height);
    int3 pixel = int3(p.uv * float2(width, height), 0);
    float depth = gSceneDepth.Load(pixel);
    uint view = (uint)gViewParams.x;
    if (depth >= kFarDepth)
    {
        if (view == kViewFinal)
        {
            discard; // 何も描かれていない所は、シーンを消した色（PostProcess の kClearColor）のまま残す
        }
        return float4(kNothingColor, 1.0);
    }
    float3 c = kNothingColor;
    if (view == kViewFinal)
    {
        float4 normalAndSharpness = gGBufferNormal.Load(pixel);
        float sharpness = normalAndSharpness.w;
        float3 specular = normalizedSpecularColor(gGBufferSpecular.Load(pixel).rgb, sharpness);
        float3 world = reconstructWorldPosition(p.uv, depth);
        c = lightSurface(gGBufferAlbedo.Load(pixel).rgb, normalAndSharpness.xyz, world, specular, sharpness);
    }
    else if (view == kViewAlbedo)
    {
        c = linearToSrgb(gGBufferAlbedo.Load(pixel).rgb);
    }
    else if (view == kViewNormal)
    {
        c = gGBufferNormal.Load(pixel).xyz * 0.5 + 0.5; // -1〜1 を 0〜1 へ
    }
    else if (view == kViewSpecular)
    {
        c = linearToSrgb(gGBufferSpecular.Load(pixel).rgb);
    }
    else if (view == kViewSharpness)
    {
        c = saturate(gGBufferNormal.Load(pixel).w / gViewParams.z);
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
    return float4(c, 1.0);
}
