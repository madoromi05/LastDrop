// 映り込み（SSR）を、画面の半分の大きさの描画先（RGBA16F）へ作る（ScreenSpaceReflection::draw。SsrDecalMaterial_Plan.txt 7-2）。
// 画素の位置から、目からの向きを法線で反射した向きへ少しずつ進み、深度より奥へ入った所（物に当たった所）を探して、
// その画素の「照らした後のシーンの色」を持ってくる。進み方は直線 ＋ 二分探索（Q5 の A）。
// 出力：rgb は「映る色 × 確からしさ」、a は確からしさ（SsrCommon.hlsli）。当たらなかった・粗い面・空は 0（混ぜるパスで空の色のまま）。
#include "GBuffer.hlsli"
#include "Noise.hlsli"
#include "SsrCommon.hlsli"

Texture2D gSceneColor : register(t6); // 照らした後のシーンの色（ScreenSpaceReflection.cpp の kSceneColorSlot）

// ScreenSpaceReflection.cpp の SsrConstants と同じ並び。
cbuffer SsrConstants : register(b1)
{
    float4 gSsrInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gSsrProjectionView[4];        // 射影×ビュー（行ごと）。進んだ点を画面へ戻す
    float4 gSsrMarch;                    // x: 計算する粗さの上限  y: 歩数  z: 一番遠くまで進む長さ（m）  w: 当たったとみなす厚さ（m）
    float4 gSsrFade;                     // x: 画面の端で薄くする幅（画面に対する割合）  y: 全体の強さ  z: ノイズのフレームの番号（TAA。Taa_Plan.txt 8 章）  w: 使わない
};

static const float kRoughnessFadeRange = 0.1; // 粗さの上限の手前この幅で、確からしさを 1 → 0 へ（つやの境目で映り込みがぷつっと切れない）
static const uint kRefineSteps = 4;           // 当たった歩と前の歩の間を詰める二分探索の回数
static const float kTowardEyeLimit = 0.5;     // 反射と「目への向き」の内積がこれより大きい（60° より目の方を向く）反射は計算しない
static const float4 kNoReflection = float4(0.0, 0.0, 0.0, 0.0);

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float viewDepthOf(float3 world)
{
    return dot(gSsrProjectionView[3], float4(world, 1.0));
}

// 点 world が、その画素に見えている物の奥へ入っているか（入っていて、奥行きの差が厚さより小さい＝当たり）。screen：点を画面へ戻した物。
bool isBehindScene(float3 screen, uint2 depthSize)
{
    float sceneDepth = ssrViewDepthAt(gSceneDepth, gSsrInverseProjectionView, gSsrProjectionView[3], int2(screen.xy * depthSize), depthSize);
    return screen.z > sceneDepth && screen.z - sceneDepth < gSsrMarch.w;
}

bool isOnScreen(float3 screen)
{
    return screen.z > 0.0 && all(screen.xy >= 0.0) && all(screen.xy < 1.0);
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    uint width, height;
    gSceneDepth.GetDimensions(width, height);
    uint2 depthSize = uint2(width, height);
    uint2 halfPixel = (uint2)p.position.xy;
    int3 pixel = int3(halfPixel * kSsrHalfStep, 0);
    float depth = gSceneDepth.Load(pixel);
    float roughness = gGBufferMaterial.Load(pixel).g;
    float maxRoughness = gSsrMarch.x;
    float4 result = kNoReflection;
    if (depth < kFarDepth && roughness < maxRoughness)
    {
        float2 uv = (pixel.xy + 0.5) / float2(depthSize);
        float3 position = worldPositionFromDepth(gSsrInverseProjectionView, uv, depth);
        // 目から画素への向き：同じ画素の一番手前（深度 0）から画素の位置へ（目の位置を別に渡さなくてよい）。
        float3 view = normalize(position - worldPositionFromDepth(gSsrInverseProjectionView, uv, 0.0));
        float3 n = normalize(gGBufferNormal.Load(pixel).xyz);
        float3 r = reflect(view, n);
        // ほぼ目の方へ戻る反射（目の後ろへ出ていく）は画面の中で当たらないので、計算しない。
        // 0（目の方へ少しでも戻る反射を全部捨てる）にすると、水たまりを 45° より下向きに見たときの映り込みが全部消える。
        if (dot(r, view) > -kTowardEyeLimit)
        {
            uint steps = max((uint)gSsrMarch.y, 1u);
            float maxDistance = gSsrMarch.z;
            // 歩き始めを画素ごとにずらす（段々の模様を消し、ぼかしで均す）。TAA のときはフレームごとにも変える（TAA が均す）。
            float jitter = interleavedGradientNoise(halfPixel, gSsrFade.z);
            float previous = 0.0;
            float hitDistance = -1.0;
            [loop]
            for (uint i = 0; i < steps; ++i)
            {
                // 先ほど歩幅を広げる（近くは細かく、遠くは粗く）。一歩目も少し離して、自分の面に当たらないようにする。
                float ratio = (i + 1.0 + jitter) / (steps + 1.0);
                float travel = maxDistance * ratio * ratio;
                float3 screen = ssrProjectToScreen(gSsrProjectionView, position + r * travel);
                if (!isOnScreen(screen))
                {
                    break; // 画面の外へ出た（映す物が無い）
                }
                if (isBehindScene(screen, depthSize))
                {
                    // 当たった歩と前の歩の間を、二分探索で詰める。
                    float lower = previous;
                    float upper = travel;
                    [unroll]
                    for (uint k = 0; k < kRefineSteps; ++k)
                    {
                        float middle = (lower + upper) * 0.5;
                        float3 middleScreen = ssrProjectToScreen(gSsrProjectionView, position + r * middle);
                        if (isBehindScene(middleScreen, depthSize))
                        {
                            upper = middle;
                        }
                        else
                        {
                            lower = middle;
                        }
                    }
                    hitDistance = upper;
                    break;
                }
                previous = travel;
            }
            if (hitDistance > 0.0)
            {
                float3 hit = ssrProjectToScreen(gSsrProjectionView, position + r * hitDistance);
                float3 color = gSceneColor.SampleLevel(gClampSampler, hit.xy, 0).rgb;
                // 確からしさ：画面の端で薄く × つやの境目で薄く × 遠くまで進んだほど薄く。
                float edge = min(min(hit.x, 1.0 - hit.x), min(hit.y, 1.0 - hit.y));
                float confidence = saturate(edge / gSsrFade.x)
                                 * saturate((maxRoughness - roughness) / kRoughnessFadeRange)
                                 * saturate(1.0 - hitDistance / maxDistance);
                result = float4(color * confidence, confidence) * gSsrFade.y;
            }
        }
    }
    return result;
}
