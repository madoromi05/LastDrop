// SSAO（物の隙間の暗がり）を、画面の半分の大きさの描画先（RG16F）へ作る（AmbientOcclusion::draw。SsaoTranslucent_Plan.txt 5 章）
// 式は法線の向きの半球で調べる SSAO（Crytek の SSAO を法線の半球にした物。John Chapman・LearnOpenGL の形）
//   画素の位置から法線の側の半球に点をいくつか置き、その点が深度より奥（物の中）に入っていれば「遮られた」と数える
// 出力：r に AO（1 で遮られていない）、g に目からの奥行き（m。カメラの向きに沿った距離）。g は後のぼかしと拡大が、
// 輪郭（奥行きが大きく違う所）をまたいで混ぜないために使う（6 章）
// 半分の大きさの 1 画素は、元の画面の 2×2 の左上の画素の深度と法線で代表する
#include "GBuffer.hlsli"
#include "DepthPosition.hlsli"
#include "Noise.hlsli"

// AmbientOcclusion.cpp の SsaoConstants と同じ並び
cbuffer SsaoConstants : register(b1)
{
    float4 gSsaoInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gSsaoProjectionView[4];        // 射影×ビュー（行ごと）。半球の点を画面へ戻す
    float4 gSsaoShape;                    // x: 半径（m）  y: 遮られたとみなす差の下限（m）  z: 遮られた量の倍率  w: AO の曲がり
    float4 gSsaoCount;                    // x: 調べる点の数  y: 画面の上の半径の上限（画面の高さに対する割合）  z: ノイズのフレームの番号（TAA）  w: 使わない
};

static const float kPi = 3.14159265;
static const float kGoldenAngle = 2.39996323;  // 黄金角（ラジアン）。点の方位を i ごとにこれだけ回すと、どの数でも偏らない
static const float kR2Step = 0.75487766;       // R2 列（Martin Roberts 2018）の 2 つ目の歩幅。高さを方位・長さの順と関係なく散らす
static const float kNearestSampleScale = 0.1;  // 一番近い点の長さ（半径に対して）。近い点を多くする（近い物の暗がりほど効くため）
static const float kSkyDistance = 10000.0;     // 空の画素の奥行きとして書く値（どの物とも近くないと分かる大きさ）
static const float kNoOcclusionValue = 1.0;
static const float kNearlyVertical = 0.99;
static const float kHalfResolutionStep = 2.0;  // 半分の大きさの 1 画素 = 元の画面の 2 画素

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

// ワールドの位置を、射影×ビューで画面の uv と奥行き（w）へ。w が 0 以下（目の後ろ）なら uv は使えない
float3 projectToScreen(float3 world)
{
    float4 p = float4(world, 1.0);
    float4 clip = float4(dot(gSsaoProjectionView[0], p), dot(gSsaoProjectionView[1], p),
                         dot(gSsaoProjectionView[2], p), dot(gSsaoProjectionView[3], p));
    float2 ndc = clip.xy / clip.w;
    return float3(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5, clip.w);
}

// 目からの奥行き（射影×ビューの 4 行目。透視ならカメラの向きに沿った距離）
float viewDepthOf(float3 world)
{
    return dot(gSsaoProjectionView[3], float4(world, 1.0));
}

// 位置 p・法線 n の面が、周りの物にどれだけ遮られているか（0〜1。1 で全部遮られた）
// angle：点の並びを法線の周りに回す角度（画素ごとに違う。ざらつきは 6 章のぼかしで均す）
// 式を替えるとき（GTAO など）は、この関数だけを差し替える
float occlusionAt(float3 p, float3 n, float angle, uint2 depthSize)
{
    float radius = gSsaoShape.x;
    float bias = gSsaoShape.y;
    uint count = max((uint)gSsaoCount.x, 1u);
    float depthP = viewDepthOf(p);
    // 画面の上の半径の上限（5-3）。射影の縦の倍率（2 行目の長さ）から、奥行き depthP で画面の高さの割合 y になる半径を出す
    // 手元の銃・腕で点が画面の端まで飛び、読む所がばらけて重くなるのを防ぐ
    float projectionScale = length(gSsaoProjectionView[1].xyz);
    radius = min(radius, gSsaoCount.y * 2.0 * depthP / projectionScale);

    // 法線を z にした向き。接線を angle だけ法線の周りに回しておく
    float3 helper = (abs(n.y) < kNearlyVertical) ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 tangent0 = normalize(cross(helper, n));
    float3 bitangent0 = cross(n, tangent0);
    float s, c;
    sincos(angle, s, c);
    float3 tangent = tangent0 * c + bitangent0 * s;
    float3 bitangent = cross(n, tangent);

    float occluded = 0.0;
    for (uint i = 0; i < count; ++i)
    {
        // 半球の向き：方位は黄金角のらせん、高さはコサインの重み（法線に近い向きほど多く。AO の積分の重みと同じ）
        float azimuth = i * kGoldenAngle;
        float u = frac(0.5 + i * kR2Step);
        float sinTheta = sqrt(u);
        float cosTheta = sqrt(1.0 - u);
        float ratio = (i + 1.0) / count;
        float scale = radius * lerp(kNearestSampleScale, 1.0, ratio * ratio);
        float3 direction = (tangent * cos(azimuth) + bitangent * sin(azimuth)) * sinTheta + n * cosTheta;
        float3 samplePosition = p + direction * scale;

        float3 screen = projectToScreen(samplePosition);
        if (screen.z <= 0.0 || any(screen.xy < 0.0) || any(screen.xy >= 1.0))
        {
            continue; // 画面の外（深度が無い）は数えない
        }
        int3 pixel = int3(screen.xy * depthSize, 0);
        float depth = gSceneDepth.Load(pixel);
        if (depth >= kFarDepth)
        {
            continue; // 空は遮らない
        }
        float depthQ = viewDepthOf(worldPositionFromDepth(gSsaoInverseProjectionView, screen.xy, depth));
        // 点より手前に物の面がある（点が物の中に入っている）なら遮られた
        // 範囲の確かめ：p から奥行きが半径より大きく離れた物（ゾンビの後ろの壁を手前の物が、など）は少なく数える
        float rangeWeight = smoothstep(0.0, 1.0, radius / max(abs(depthP - depthQ), bias));
        occluded += (depthQ < screen.z - bias) ? rangeWeight : 0.0;
    }
    return occluded / count;
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    uint width, height;
    gSceneDepth.GetDimensions(width, height);
    uint2 depthSize = uint2(width, height);
    uint2 halfPixel = (uint2)p.position.xy;
    int3 pixel = int3(halfPixel * (uint)kHalfResolutionStep, 0);
    float depth = gSceneDepth.Load(pixel);
    if (depth >= kFarDepth)
    {
        return float4(kNoOcclusionValue, kSkyDistance, 0.0, 1.0);
    }
    float2 uv = (pixel.xy + 0.5) / float2(depthSize);
    float3 position = worldPositionFromDepth(gSsaoInverseProjectionView, uv, depth);
    float3 normal = normalize(gGBufferNormal.Load(pixel).xyz);
    float angle = interleavedGradientNoise(halfPixel, gSsaoCount.z) * 2.0 * kPi; // TAA のときはフレームごとに回し方を変える（TAA が均す）
    float occluded = occlusionAt(position, normal, angle, depthSize);
    float ao = pow(saturate(1.0 - occluded * gSsaoShape.z), gSsaoShape.w);
    return float4(ao, viewDepthOf(position), 0.0, 1.0);
}
