// TAA（時間をまたいだアンチエイリアス。TemporalAntiAliasing::resolve。SummaryPlan/個別仕様/グラフィック/Taa_Plan.txt 7 章）
// 今のシーンの色（毎フレーム 1 画素より細かくずらして描いた物。TemporalJitter）と、前のフレームまでに混ぜた履歴を混ぜて、新しい履歴を書く
// 何フレームかの色が混ざるので、1 画素の中を細かく調べたのと同じになり、縁のギザギザと、細い物のちらつきが消える
//   前のフレームのどこを読むか：G-Buffer の RT1 の a（動きの種類。Taa_Plan.txt 6 章）で分ける
//     動かない物・動く物 … 深度から位置を戻し、前のフレームの「ずらしていない射影×ビュー」で画面へ戻した所
//     手元（自キャラ・銃・弾）… カメラと一緒に動くので、同じ画素
//   残像を消す：今の画素の周り 3×3 の色の平均と広がり（YCoCg）で箱を作り、履歴の色をその箱の中へ寄せる（Salvi の分散で切る方法）
//   混ぜる重みは明るさで割り戻す（Karis。HDR の明るすぎる 1 画素が履歴に焼き付いて点滅しないように）
#include "GBuffer.hlsli"
#include "DepthPosition.hlsli"

Texture2D gCurrent : register(t6); // 今のシーンの色（TemporalAntiAliasing.cpp の kCurrentSlot。霧の後・半透明の板の前）
Texture2D gHistory : register(t7); // 前の履歴（同 kHistorySlot）

// TemporalAntiAliasing.cpp の TaaConstants と同じ並び
cbuffer TaaConstants : register(b1)
{
    float4 gTaaInverseProjectionView[4];  // 今の（ずらした）射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gTaaPreviousProjectionView[4]; // 前のフレームの「ずらしていない」射影×ビュー（行ごと）
    float4 gTaaHistoryWeight;             // x: 動かない物  y: 動く物  z: 手元（履歴を混ぜる割合）  w: 1 なら履歴を捨てる
    float4 gTaaClipGamma;                 // x: 動かない物  y: 動く物  z: 手元（箱の広さ。標準偏差の何倍か）  w: 使わない
};

static const float kMotionMovingThreshold = 0.25;      // RT1 の a がこれより大きければ動く物（GBuffer.cpp の kMotionValues：0・0.5・1）
static const float kMotionFirstPersonThreshold = 0.75; // これより大きければ手元
static const float kNeighborCount = 9.0;               // 周り 3×3
static const float kClipEpsilon = 1e-4;                // 箱の幅が 0 のときに 0 で割らない
static const float3 kLumaWeights = float3(0.2126, 0.7152, 0.0722); // Rec.709 の明るさ

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float3 rgbToYCoCg(float3 c)
{
    return float3(dot(c, float3(0.25, 0.5, 0.25)), dot(c, float3(0.5, 0.0, -0.5)), dot(c, float3(-0.25, 0.5, -0.25)));
}

float3 yCoCgToRgb(float3 c)
{
    return float3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

// 前の履歴を Catmull-Rom（なめらかな 3 次の補間）で読む。双線形で読むと、読み直すたびに 1 段ずつぼけていくため
// 4×4 の 16 画素を、双線形の読み込み 5 回で近似する（四隅の 4 つは重みが小さいので読まない。Jimenez・Pettineo の方法）
float3 sampleHistoryCatmullRom(float2 uv, float2 size)
{
    float2 position = uv * size;
    float2 center = floor(position - 0.5) + 0.5;
    float2 f = position - center;
    float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    float2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    float2 w3 = f * f * (-0.5 + 0.5 * f);
    float2 w12 = w1 + w2;
    float2 uv0 = (center - 1.0) / size;
    float2 uv12 = (center + w2 / w12) / size;
    float2 uv3 = (center + 2.0) / size;
    float3 sum = gHistory.SampleLevel(gClampSampler, float2(uv12.x, uv0.y), 0).rgb * (w12.x * w0.y)
               + gHistory.SampleLevel(gClampSampler, float2(uv0.x, uv12.y), 0).rgb * (w0.x * w12.y)
               + gHistory.SampleLevel(gClampSampler, uv12, 0).rgb * (w12.x * w12.y)
               + gHistory.SampleLevel(gClampSampler, float2(uv3.x, uv12.y), 0).rgb * (w3.x * w12.y)
               + gHistory.SampleLevel(gClampSampler, float2(uv12.x, uv3.y), 0).rgb * (w12.x * w3.y);
    float weight = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
    return max(sum / weight, 0.0); // 3 次の補間は負の重みがあるので、0 より下へ行かないよう止める
}

// 履歴の色 history を、箱（中心 center・半分の幅 extent）の中へ、中心へ向かって寄せる（箱の外なら、箱の面との交わりまで）
float3 clipToBox(float3 history, float3 center, float3 extent)
{
    float3 offset = history - center;
    float3 ratio = abs(offset / max(extent, kClipEpsilon));
    float largest = max(ratio.x, max(ratio.y, ratio.z));
    return (largest > 1.0) ? center + offset / largest : history;
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    uint width, height;
    gCurrent.GetDimensions(width, height);
    float2 size = float2(width, height);
    int2 pixel = (int2)p.position.xy;
    int2 last = int2(width, height) - 1;
    float3 current = gCurrent.Load(int3(pixel, 0)).rgb;
    float3 result = current;
    if (gTaaHistoryWeight.w < 0.5)
    {
        // 動きの種類ごとの、履歴を混ぜる割合と箱の広さ
        float motion = gGBufferNormal.Load(int3(pixel, 0)).a;
        bool firstPerson = motion > kMotionFirstPersonThreshold;
        bool moving = !firstPerson && motion > kMotionMovingThreshold;
        float historyWeight = firstPerson ? gTaaHistoryWeight.z : (moving ? gTaaHistoryWeight.y : gTaaHistoryWeight.x);
        float gamma = firstPerson ? gTaaClipGamma.z : (moving ? gTaaClipGamma.y : gTaaClipGamma.x);

        // 前のフレームの位置。手元は同じ画素（カメラと一緒に動く）。それ以外は深度から戻した位置を前の画面へ戻す
        float2 uv = (pixel + 0.5) / size;
        float2 previousUv = uv;
        if (!firstPerson)
        {
            float depth = gSceneDepth.Load(int3(pixel, 0));
            float3 world = worldPositionFromDepth(gTaaInverseProjectionView, uv, depth);
            float4 position = float4(world, 1.0);
            float4 clip = float4(dot(gTaaPreviousProjectionView[0], position), dot(gTaaPreviousProjectionView[1], position),
                                 dot(gTaaPreviousProjectionView[2], position), dot(gTaaPreviousProjectionView[3], position));
            float2 ndc = clip.xy / clip.w;
            previousUv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
        }
        // 前の位置が画面の外（今のフレームで初めて見えた所）は、今の色だけ
        if (all(previousUv >= 0.0) && all(previousUv < 1.0))
        {
            // 周り 3×3 の色の平均と広がり（YCoCg）
            float3 sum = 0.0;
            float3 sumSquared = 0.0;
            [unroll]
            for (int y = -1; y <= 1; ++y)
            {
                [unroll]
                for (int x = -1; x <= 1; ++x)
                {
                    float3 neighbor = rgbToYCoCg(gCurrent.Load(int3(clamp(pixel + int2(x, y), int2(0, 0), last), 0)).rgb);
                    sum += neighbor;
                    sumSquared += neighbor * neighbor;
                }
            }
            float3 mean = sum / kNeighborCount;
            float3 deviation = sqrt(abs(sumSquared / kNeighborCount - mean * mean));
            float3 history = sampleHistoryCatmullRom(previousUv, size);
            history = yCoCgToRgb(clipToBox(rgbToYCoCg(history), mean, deviation * gamma));
            // 明るさで割り戻した重みで混ぜる
            float currentWeight = (1.0 - historyWeight) / (1.0 + dot(current, kLumaWeights));
            float historyWeighted = historyWeight / (1.0 + dot(history, kLumaWeights));
            result = (current * currentWeight + history * historyWeighted) / (currentWeight + historyWeighted);
        }
    }
    return float4(result, 1.0);
}
