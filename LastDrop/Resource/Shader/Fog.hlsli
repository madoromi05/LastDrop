// 霧（SummaryPlan/個別仕様/グラフィック/NightFinish_Plan.txt 5 章）。霧のパス（Fog.hlsl）・半透明の板（LitQuad.hlsl）・G-Buffer の表示（DeferredLighting.hlsl）が使う
// 霧の値はライトの一覧（LightList.hlsli の gFog）、目の位置は gEyePosition から読む
//   距離の霧：( d × k )²（2 乗の指数。Direct3D 9 の FOG_EXP2 と同じ形）。近くは薄いまま、奥で急に濃くなる
//   高さの霧：濃さ ρ(y) = 濃さ × exp( −b × y ) を、目から位置までの線の上で足し合わせる（Inigo Quilez「Better Fog」）
//   霧の量 = 1 − exp( −( 距離の霧 ＋ 高さの霧 ) )
// 霧の色は空の地平線の色（描いている空と同じ明るさ）。空は地平線へ向かってこの色になるので、奥の建物が空へ溶ける
#ifndef INCLUDED_FOG_HLSLI
#define INCLUDED_FOG_HLSLI

#include "Lighting.hlsli"
#include "LightList.hlsli"

static const float kFlatRayLimit = 1e-4;	// 目と位置の高さの差（× b）がこれより小さい線は水平として扱う（足し合わせの式の割り算が 0 割りに近づくため）

// 位置 position（ワールド）が、目から見てどれだけ霧に埋もれるか（0 で霧なし、1 で霧の色だけ）
float fogAmount(float3 position)
{
    float3 toPoint = position - gEyePosition.xyz;
    float distance = length(toPoint);
    float scaledDistance = distance * gFog.x;
    float opticalDepth = scaledDistance * scaledDistance;
    // 高さの霧：目の高さでの薄まり方 exp(−b × 目の高さ) から、位置の高さでの薄まり方までの平均 × 距離
    float falloff = gFog.z;
    float eyeFalloff = exp(-falloff * gEyePosition.y);
    float heightChange = falloff * toPoint.y;
    float averageFalloff = (abs(heightChange) > kFlatRayLimit)
        ? (eyeFalloff - exp(-falloff * position.y)) / heightChange
        : eyeFalloff;
    opticalDepth += gFog.y * distance * averageFalloff;
    return 1.0 - exp(-opticalDepth);
}

// 霧の色（リニア）。空の地平線の色に、描く空の明るさを掛けた物（LightList.hlsli の skyColor の地平線と同じ数）
float3 fogColor()
{
    return srgbToLinear(gSkyHorizon.rgb) * gSkyZenith.w;
}

// 位置 position の色 color（リニア）に霧を掛ける
float3 applyFog(float3 color, float3 position)
{
    return lerp(color, fogColor(), fogAmount(position));
}

#endif
