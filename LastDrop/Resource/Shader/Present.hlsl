// 中間の描画先に描いたシーンを、画面へ写す（PostProcess::endScene）。
// 明るい所のにじみ（ブルーム。Bloom::apply が作った絵）を足してから、周辺を少し暗くする（ビネット）。
// 噛まれた瞬間は周辺を赤へ寄せ・ぼかし・暗くし（真ん中は触らない）、死んだ後は全体を暗くする（PostProcess::setDamageFlash・setPain・setFade）。
// トーンマップ・カラーグレーディングなどはここへ足していく（Graphics_Spec.txt 2 章の (10)）。
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
    float3 gFlashColor;      // 赤みの色
    float gPadding;
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

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex v) : SV_Target
{
    float distance = length(v.uv - 0.5) * 2.0;
    float3 color = sampleScene(v.uv, distance);
    // 1 を超えた所は、この後バックバッファへ書くときに 1 で切られる（トーンマップはまだ無い。Bloom_Plan.txt 6 章）。
    color += gBloom.Sample(gClampSampler, v.uv).rgb * gBloomIntensity;
    float darken = (gVignetteStrength + gPainDarken * gPain) * smoothstep(gVignetteInner, gVignetteOuter, distance);
    color *= 1.0 - darken;
    float flash = gDamageFlash * smoothstep(gFlashInner, gFlashOuter, distance);
    color = lerp(color, gFlashColor, saturate(flash));
    return float4(color * (1.0 - saturate(gFade)), 1.0);
}
