// 中間の描画先に描いたシーンを、画面へ写す（PostProcess::endScene）。
// シーンはリニア（光の量に比例した数）で描いてある。明るい所のにじみ（ブルーム。Bloom::apply が作った絵）を足し、
// 露出を掛けて ACES で 0〜1 に収め（トーンマップ）、周辺を少し暗くし（ビネット）、最後に画面の色（sRGB）へ戻す。
// 順番と理由は SummaryPlan/個別仕様/LinearToneMap_Plan.txt 6 章。
// 噛まれた瞬間は周辺を赤へ寄せ・ぼかし・暗くし（真ん中は触らない）、死んだ後は全体を暗くする（PostProcess::setDamageFlash・setPain・setFade）。
// カラーグレーディング・ディザはトーンマップの後・画面の色へ戻す前へ足していく（Graphics_Spec.txt 2 章の (10)）。
#include "Common.hlsli"

Texture2D gSceneColor : register(t2); // PostProcess.cpp の kSceneColorSlot
Texture2D gBloom      : register(t3); // PostProcess.cpp の kBloomSlot（画面の 1/2 の大きさ）

cbuffer PresentConstants : register(b1) // PostProcess.cpp の PresentConstants と同じ並び
{
    float gVignetteStrength; // 角での暗くなり具合（0で無し、1で真っ黒）
    float gVignetteInner;    // 暗くなり始める距離（画面中心0〜上下左右の端1）
    float gVignetteOuter;    // 一番暗くなる距離（角は約1.41）
    float gBloomIntensity;   // ブルームを足す強さ
    float gDamageFlash;      // 噛まれた瞬間の赤みの強さ（0で無し、1で一番赤い）
    float gFlashInner;       // 赤くなり始める距離（画面中心0〜上下左右の端1）
    float gFlashOuter;       // 一番赤くなる距離
    float gFade;             // 画面全体を暗くする量（0でそのまま、1で真っ黒。死んだ後の暗転）
    float3 gFlashColor;      // 赤みの色（画面の色。混ぜる前にリニアへ直す）
    float gExposure;         // トーンマップの前にシーンへ掛ける倍率（PostProcessParams::exposure）
    float gPain;             // 噛まれた瞬間の痛みの強さ（0で無し、1で一番強い）。周りのぼかしと暗さ
    float gPainBlurLength;   // 痛み1・画面の端で、中心までの距離の何割の長さを読んでぼかすか
    float gPainBlurInner;    // ぼかし始める距離（画面中心0〜上下左右の端1）。これより内（照準の周り）はぼかさない
    float gPainDarken;       // 痛み1のとき、ビネットの暗さに足す量
};

// ぼかすときに読む回数。多いほどなめらかだが重い（痛みのある 0.6 秒だけ読む）。
static const int kPainBlurTaps = 8;

// 画面の中心へ向かう線の上を kPainBlurTaps 回読んで平均する（放射状のぼかし）。強さ 0 なら 1 回だけ読む。
float3 sampleScene(float2 uv, float distance)
{
    float strength = gPain * smoothstep(gPainBlurInner, 1.0, distance);
    float3 color = gSceneColor.SampleLevel(gClampSampler, uv, 0).rgb;
    [branch]
    if (strength > 0.0)
    {
        float2 toCenter = (0.5 - uv) * gPainBlurLength * strength;
        // 1 回目（t = 0）は上で読んだ色なので、残りを読んで足す。
        for (int i = 1; i < kPainBlurTaps; ++i)
        {
            float t = (float)i / (float)(kPainBlurTaps - 1);
            color += gSceneColor.SampleLevel(gClampSampler, uv + toCenter * t, 0).rgb;
        }
        color /= (float)kPainBlurTaps;
    }
    return color;
}

// ACES のフィルム調の曲線の近似（Krzysztof Narkowicz, 2016）。0〜無限 のリニアの明るさを 0〜1 へなだらかに収める。
// 1 で切るのと違い、明るい所が急に白く飛ばず、マズルフラッシュの芯のような強い光にも色が残る。
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
static const float kAcesA = 2.51;
static const float kAcesB = 0.03;
static const float kAcesC = 2.43;
static const float kAcesD = 0.59;
static const float kAcesE = 0.14;
float3 toneMapAces(float3 x)
{
    return saturate((x * (kAcesA * x + kAcesB)) / (x * (kAcesC * x + kAcesD) + kAcesE));
}

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex v) : SV_Target
{
    float distance = length(v.uv - 0.5) * 2.0;
    float3 color = sampleScene(v.uv, distance);
    color += gBloom.Sample(gClampSampler, v.uv).rgb * gBloomIntensity;
    color = toneMapAces(color * gExposure);
    // ここから先は 0〜1 の色に掛ける（トーンマップの前に掛けると、明るい所ほど効き方が変わるため）。
    float darken = (gVignetteStrength + gPainDarken * gPain) * smoothstep(gVignetteInner, gVignetteOuter, distance);
    color *= 1.0 - darken;
    float flash = gDamageFlash * smoothstep(gFlashInner, gFlashOuter, distance);
    color = lerp(color, srgbToLinear(gFlashColor), saturate(flash));
    color *= 1.0 - saturate(gFade);
    // バックバッファは UNORM のまま（HUD・文字を今の色で描くため）なので、画面の色へは自分で戻す。
    return float4(linearToSrgb(color), 1.0);
}
