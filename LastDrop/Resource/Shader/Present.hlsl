// 中間の描画先に描いたシーンを、画面へ写す（PostProcess::endScene）
// シーンはリニア（光の量に比例した数）で描いてある。明るい所のにじみ（ブルーム。Bloom::apply が作った絵）を足し、
// 露出を掛けて ACES で 0〜1 に収め（トーンマップ）、周辺を少し暗くし（ビネット）、最後に画面の色（sRGB）へ戻す
// 順番と理由は SummaryPlan/個別仕様/グラフィック/LinearToneMap_Plan.txt 6 章
// 噛まれた瞬間は周辺を赤へ寄せ・ぼかし・暗くし（真ん中は触らない）、死んだ後は全体を暗くする（PostProcess::setDamageFlash・setPain・setFade）
// トーンマップの直後にカラーグレーディング（夜の色）を掛け、画面の色（8bit）へ落とす直前にディザ（縞を消す弱いノイズ）を足す
// グレーディングはビネット・赤みより前（噛まれた赤みを青寄りの色温度で濁らせない）
// グレーディングの後に、読んであれば色の表（LUT。外の道具で直した色）を混ぜる
#include "Common.hlsli"
#include "Noise.hlsli" // ディザの interleavedGradientNoise（SSAO と共有）
#include "Grading.hlsli" // 定数（PresentConstants.hlsli）・gradeColor・色の表（LUT）。表を焼く LutBake.hlsl と共有

Texture2D gSceneColor : register(t2); // PostProcess.cpp の kSceneColorSlot
Texture2D gBloom      : register(t3); // PostProcess.cpp の kBloomSlot（画面の 1/2 の大きさ）

static const int kPainBlurTaps = 8;	// ぼかすときに読む回数。多いほどなめらかだが重い（痛みのある 0.6 秒だけ読む）

// 画面の中心へ向かう線の上を kPainBlurTaps 回読んで平均する（放射状のぼかし）。強さ 0 なら 1 回だけ読む
float3 sampleScene(float2 uv, float distance)
{
    float strength = gPain * smoothstep(gPainBlurInner, 1.0, distance);
    float3 color = gSceneColor.SampleLevel(gClampSampler, uv, 0).rgb;
    [branch]
    if (strength > 0.0)
    {
        float2 toCenter = (0.5 - uv) * gPainBlurLength * strength;
        // 1 回目（t = 0）は上で読んだ色なので、残りを読んで足す
        for (int i = 1; i < kPainBlurTaps; ++i)
        {
            float t = (float)i / (float)(kPainBlurTaps - 1);
            color += gSceneColor.SampleLevel(gClampSampler, uv + toCenter * t, 0).rgb;
        }
        color /= (float)kPainBlurTaps;
    }
    return color;
}

// ACES のフィルム調の曲線の近似（Krzysztof Narkowicz, 2016）。0〜無限 のリニアの明るさを 0〜1 へなだらかに収める
// 1 で切るのと違い、明るい所が急に白く飛ばず、マズルフラッシュの芯のような強い光にも色が残る
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

static const float3 kScreenLumaWeights = float3(0.299, 0.587, 0.114);	// 画面の色（sRGB の数）の輝度の重み（Rec.601。FXAA 3.11 と同じく、見た目の明るさの差で輪郭を探すため）

// ── ディザ ──
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
    // ここから先は 0〜1 の色に掛ける（トーンマップの前に掛けると、明るい所ほど効き方が変わるため）
    color = gradeColorWithLut(color);
    float darken = (gVignetteStrength + gPainDarken * gPain) * smoothstep(gVignetteInner, gVignetteOuter, distance);
    color *= 1.0 - darken;
    float flash = gDamageFlash * smoothstep(gFlashInner, gFlashOuter, distance);
    color = lerp(color, srgbToLinear(gFlashColor), saturate(flash));
    color *= 1.0 - saturate(gFade);
    // バックバッファは UNORM のまま（HUD・文字を今の色で描くため）なので、画面の色へは自分で戻す
    // 8bit に落とす直前に ±半段のノイズを足し、夜空・霧のなだらかな暗い色の縞（バンディング）を消す
    float dither = (interleavedGradientNoise(v.position.xy, gNoiseFrame) - 0.5) * gDitherAmount;
    float3 screenColor = linearToSrgb(color) + dither;
    // a には画面の色の輝度を入れる。FXAA（Fxaa.hlsl）が近くの輝度を何度も読むので、ここで 1 回だけ計算しておく
    // （FXAA を切って画面へ直接写すときは、画面の a は使われない）
    return float4(screenColor, dot(saturate(screenColor), kScreenLumaWeights));
}
