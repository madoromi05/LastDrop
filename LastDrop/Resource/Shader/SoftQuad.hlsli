// 半透明の板を、壁・地面に刺さった所で薄くする（ソフトパーティクル。SsaoTranslucent_Plan.txt 7 章）
// LitQuad.hlsl（砂ぼこり・血しぶき）と GlowQuad.hlsl（マズルフラッシュ）が使う。弾痕は面に貼る板なので使わない（薄くする距離 0）
// 板の画素と、そこに見えている物の、目からの奥行きの差が小さいほど薄くする
// 深度は PostProcess::resumeSceneReadOnlyDepth が読み取り専用で付け、t5 から読めるようにしている（奥行きの判定はそのまま GPU が行う）
#ifndef INCLUDED_SOFT_QUAD_HLSLI
#define INCLUDED_SOFT_QUAD_HLSLI

#include "DepthPosition.hlsli"

// LitQuadBatch.cpp の SoftQuadConstants と同じ並び
cbuffer SoftQuadConstants : register(b1)
{
    float4 gSoftInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gSoftViewDepthRow;             // 射影×ビューの 4 行目。位置との内積が目からの奥行き（m）
    float4 gSoftParams;                   // x: 薄くし始める奥行きの差（m。0 以下なら薄くしない）  yzw: 使わない
};

Texture2D<float> gSoftSceneDepth : register(t5); // PostProcess.cpp の kSoftQuadDepthSlot

// 0〜1（0 で消える、1 でそのまま）。position：SV_Position（xy が画素の位置）、world：板の画素のワールドの位置
// return は 1 か所にする（途中で返すと、最適化なしの fxc が「初期化していないかもしれない」と警告するため。Pbr.hlsli と同じ）
float softFade(float4 position, float3 world)
{
    float fade = 1.0;
    float softDistance = gSoftParams.x;
    // 薄くしない板は深度を読まない（付いていないこともある）
    if (softDistance > 0.0)
    {
        uint width, height;
        gSoftSceneDepth.GetDimensions(width, height);
        float depth = gSoftSceneDepth.Load(int3(position.xy, 0));
        // 後ろが空なら何にも刺さらない（薄くしない）
        if (depth < kFarDepth)
        {
            float2 uv = position.xy / float2(width, height);
            float3 scene = worldPositionFromDepth(gSoftInverseProjectionView, uv, depth);
            float difference = dot(gSoftViewDepthRow, float4(scene, 1.0)) - dot(gSoftViewDepthRow, float4(world, 1.0));
            fade = saturate(difference / softDistance);
        }
    }
    return fade;
}

#endif
