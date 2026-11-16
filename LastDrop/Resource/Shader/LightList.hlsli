// 1 フレームのライトの一覧（PbrNightLighting_Plan.txt 6 章）。GameSrc/Render/LightList.cpp の LightConstants と同じ並び
// 読むのはライティングのパス（DeferredLighting.hlsl）と半透明の板（LitQuad.hlsl）。どちらも Pbr.hlsli の shadeSurface から
#ifndef INCLUDED_LIGHT_LIST_HLSLI
#define INCLUDED_LIGHT_LIST_HLSLI

#include "Common.hlsli"

static const uint kMaxLights = 30; // LightList.h の kMaxLights と揃える
static const uint kMaxShadowMaps = 4; // LightList.h の kMaxShadowMaps と揃える

// LightList.h の LightKind と同じ番号
static const uint kLightDirectional = 0;
static const uint kLightPoint       = 1;
static const uint kLightSpot        = 2;

struct LightData
{
    float4 positionRange;   // xyz 位置（点・スポット）  w 届く距離（m）
    float4 directionKind;   // xyz 光の進む向き（平行光・スポット）  w 種類
    float4 colorIntensity;  // xyz 色（sRGB。読むときにリニアへ直す）  w 強さ
    float4 spotScaleOffset; // x 角度の倍率  y 角度のずらし（saturate( cos × x + y ) が内側で 1・外側で 0）  z 影のマップの番号（無ければ −1）  w 影のマップの枚数（2 以上なら CSM）
    float4 nearDistance;    // x 近さの下限（m。これより近い面はこの距離と同じ明るさ。LightList.h の LightSource::nearDistance）  yzw 使わない
};

// b2（ShaderSlots::kLightConstants）。b1 は全画面のパスの定数・キャラの骨、b3 は G-Buffer の材質値
cbuffer LightConstants : register(b2)
{
    float4 gLightCount; // x: 照らすライトの数
    float4 gSkyZenith;  // xyz 天頂の色（sRGB）  w 描く空の明るさ
    float4 gSkyHorizon; // xyz 地平線の色（sRGB）  w 環境光の強さ
    float4 gGround;     // xyz 地面の色（sRGB。下からの環境光）
    float4 gFog;        // x 距離の霧の係数 k  y 高さ 0 での高さの霧の濃さ（1/m）  z 高さで薄まる係数 b（Fog.hlsli。LightList.cpp の fogConstants）
    float4 gMoonDirection; // xyz 月の方への向き（長さ 1）  w 見かけの半径（ラジアン）。SkyDetail.hlsli
    float4 gMoonColor;     // xyz 月の色（sRGB）  w 円板の明るさ（0 なら描かない）
    float4 gMoonHalo;      // x にじみの明るさ（円板に対する割合）  y にじみの広がり（半径の倍数）  z 画面の 1 画素の角度（ラジアン。月の模様のミップを選ぶ）
    float4 gShadowSettings;                      // x 法線のずらし（画素）  y 光の方へのずらし（画素）  z 影の濃さ
    float4 gCascadeEnds;                         // xyz 月の CSM の各枚の奥の端（m）  w 次の枚と混ぜる割合
    float4 gCascadeView;                         // xyz カメラの向き（奥行きを測る向き）
    float4 gShadowMatrices[kMaxShadowMaps * 4];  // 影のマップごとに、光の側の射影×ビュー（行ごとに 4 つ）
    float4 gShadowTexels[kMaxShadowMaps];        // x 1 画素の大きさ（m）  y 1 なら透視（x は光から 1m の所の大きさ）
    LightData gLights[kMaxLights];
};

// 影のマップ（LightList::bindShadowMaps）。番号 0 は懐中電灯、1〜3 は月（段階 4-b）
// 付いていない番号は、ライトの shadowMap がその番号を指さないので読まれない
Texture2D<float> gShadowMap0 : register(t6);
Texture2D<float> gShadowMap1 : register(t7);
Texture2D<float> gShadowMap2 : register(t8);
Texture2D<float> gShadowMap3 : register(t9);

// PCF（影の縁をなめらかにする）。比べる位置を 1 画素ずつずらして 3×3 回読み、平均する。比較サンプラ（s2）自体も 2×2 を混ぜる
static const int kPcfRadius = 1;
static const float kPcfSampleCount = 9.0;
static const float kNoShadow = 1.0;

// index 番の影のマップで、uv の所の深度と depth を比べた割合（1 で光が届く）。番号は動的なので、ここで振り分ける
// （return を 1 か所にする。途中で返すと fxc が「初期化していないかもしれない」と警告するため。Pbr.hlsli の evaluateLight と同じ）
float compareShadowMap(uint index, float2 uv, float depth)
{
    float lit;
    if (index == 0) { lit = gShadowMap0.SampleCmpLevelZero(gShadowSampler, uv, depth); }
    else if (index == 1) { lit = gShadowMap1.SampleCmpLevelZero(gShadowSampler, uv, depth); }
    else if (index == 2) { lit = gShadowMap2.SampleCmpLevelZero(gShadowSampler, uv, depth); }
    else { lit = gShadowMap3.SampleCmpLevelZero(gShadowSampler, uv, depth); }
    return lit;
}

float2 shadowMapSize(uint index)
{
    uint width, height;
    if (index == 0) { gShadowMap0.GetDimensions(width, height); }
    else if (index == 1) { gShadowMap1.GetDimensions(width, height); }
    else if (index == 2) { gShadowMap2.GetDimensions(width, height); }
    else { gShadowMap3.GetDimensions(width, height); }
    return float2(width, height);
}

// 位置 position（法線 normal・光の方 toLight・光までの距離 lightDistance）に、index 番の影のマップのライトの光がどれだけ届くか（0〜1）
// にきび対策：読む位置を法線の向きと光の方へ、影のマップの画素の大きさの何倍かだけずらす
float shadowVisibility(uint index, float3 position, float3 normal, float3 toLight, float lightDistance)
{
    float4 texel = gShadowTexels[index];
    float texelSize = (texel.y > 0.5) ? texel.x * lightDistance : texel.x;
    float3 p = position + normal * (gShadowSettings.x * texelSize) + toLight * (gShadowSettings.y * texelSize);
    uint row = index * 4;
    float4 clip = float4(dot(gShadowMatrices[row + 0], float4(p, 1.0)), dot(gShadowMatrices[row + 1], float4(p, 1.0)),
                         dot(gShadowMatrices[row + 2], float4(p, 1.0)), dot(gShadowMatrices[row + 3], float4(p, 1.0)));
    if (clip.w <= 0.0)
    {
        return kNoShadow; // 光の後ろ（スポットの照らす向きの逆）。光は届かないが、それはスポットの角度の減衰が 0 にする
    }
    float3 ndc = clip.xyz / clip.w;
    float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
    if (any(uv < 0.0) || any(uv > 1.0) || ndc.z > 1.0)
    {
        return kNoShadow; // 影のマップの外（届く距離の外・角度の外）は影を付けない
    }
    float2 uvStep = 1.0 / shadowMapSize(index);
    float sum = 0.0;
    for (int y = -kPcfRadius; y <= kPcfRadius; ++y)
    {
        for (int x = -kPcfRadius; x <= kPcfRadius; ++x)
        {
            sum += compareShadowMap(index, uv + float2(x, y) * uvStep, ndc.z);
        }
    }
    return lerp(kNoShadow, sum / kPcfSampleCount, gShadowSettings.z);
}

// 月の CSM。first から続く 3 枚のうち、カメラからの奥行き（カメラの向きに沿った距離）で枚を選んで影を読む
// 各枚の受け持ちの奥の gCascadeEnds.w 割は、次の枚と混ぜる（枚の境目で影の細かさが急に変わって見えないように）
// 一番奥の枚の端より先は影を付けない（同じく端の手前で影を薄くしていく）。eye：カメラの目の位置
float cascadedShadowVisibility(uint first, float3 position, float3 normal, float3 toLight, float3 eye)
{
    float depth = dot(position - eye, gCascadeView.xyz);
    float3 ends = gCascadeEnds.xyz;
    if (depth >= ends.z)
    {
        return kNoShadow;
    }
    uint cascade = (depth < ends.x) ? 0 : ((depth < ends.y) ? 1 : 2);
    float start = (cascade == 0) ? 0.0 : ((cascade == 1) ? ends.x : ends.y);
    float end = (cascade == 0) ? ends.x : ((cascade == 1) ? ends.y : ends.z);
    float visibility = shadowVisibility(first + cascade, position, normal, toLight, 1.0);
    float blendStart = end - (end - start) * gCascadeEnds.w;
    if (depth > blendStart)
    {
        // 次の枚（一番奥の枚なら影なし）へなめらかに移す
        float next = (cascade < 2) ? shadowVisibility(first + cascade + 1, position, normal, toLight, 1.0) : kNoShadow;
        visibility = lerp(visibility, next, saturate((depth - blendStart) / max(end - blendStart, 1e-4)));
    }
    return visibility;
}

static const float kSkyGradientPower = 0.5;	// 地平線（向きの y = 0）から天頂（y = 1）までの色の変わり方。1 より小さいと、地平線の近くだけ明るく残り、すぐ天頂の色になる
static const float kSkyAverageBlend = 0.5;	// 空の平均の色（拡散の環境光の上半分）は、地平線の色と天頂の色の真ん中

// 向き dir（長さ 1）に見える空の色（リニア。明るさは掛けない）。地平線より下は地平線の色のまま
float3 skyGradient(float3 dir)
{
    float t = pow(saturate(dir.y), kSkyGradientPower);
    return lerp(srgbToLinear(gSkyHorizon.rgb), srgbToLinear(gSkyZenith.rgb), t);
}

#include "SkyDetail.hlsli"

// 描く空の色（向き dir。長さ 1）。グラデーションに月の絵（模様つき）を足す
// 月は空の明るさ（gSkyZenith.w）を掛けない：グラデーションを暗くしても、月の明るさは自分の値のまま
float3 skyColor(float3 dir)
{
    return skyGradient(dir) * gSkyZenith.w + moonWithTexture(dir);
}

// 向き dir から来る環境光（リニア。強さ込み）。上半分は空、下半分は地面。地平線をまたぐ所はなめらかにつなぐ
// roughness：0 なら dir の方の色そのもの、1 なら半球全体をならした色（粗い面の映り込み・拡散の環境光はこちら）
// つるつるの面には月の円板も映す（水たまり・車の塗装）
float3 environmentLight(float3 dir, float roughness)
{
    float3 ground = srgbToLinear(gGround.rgb);
    float3 skyAverage = lerp(srgbToLinear(gSkyHorizon.rgb), srgbToLinear(gSkyZenith.rgb), kSkyAverageBlend);
    float3 blurred = lerp(ground, skyAverage, dir.y * 0.5 + 0.5);    // 半球ライト（-1〜1 を 0〜1 へ。上を向くほど空）
    float3 sharp = (dir.y >= 0.0) ? skyGradient(dir) : ground;       // その向きにちょうど見える物
    float3 moon = (dir.y >= 0.0) ? reflectedMoon(dir, roughness) : 0.0;
    return lerp(sharp, blurred, roughness) * gSkyHorizon.w + moon;
}

#endif
