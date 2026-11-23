// FXAA（PostProcess::drawFxaa）。写した結果（8bit の中間。a に輝度）を読み、
// 輪郭のギザギザを、輪郭を横切る向きに隣の画素と混ぜてなめらかにしながら画面へ写す
// 手順は FXAA 3.11 の Quality 版（Timothy Lottes, NVIDIA）と同じ。探す回数は Quality の 12 番（5 回）にした
//   1) 真ん中と上下左右の輝度の差が小さければ、そのまま返す（平らな所。画面の大半はここで終わる）
//   2) 斜めの 4 つも読み、輪郭が横向きか縦向きかを決める
//   3) 輪郭に沿って両方向へ、輝度が「輪郭の上の平均」から大きく離れる所（輪郭の端）まで探す
//   4) 近い方の端までの距離から、輪郭を横切る向きにどれだけずらして読むかを決める（端に近いほど大きく）
//   5) 細かい模様（1 画素の点や線）は、周りの平均との差から別にずらす量を出し、4) と大きい方を使う
//   6) ずらした位置を線形で読む（隣の画素と混ざる）
// 中間も画面も画面の色（sRGB の数）のまま。ここでは直さない
#include "Common.hlsli"

Texture2D gInput : register(t2); // PostProcess.cpp の kFxaaInputSlot

cbuffer FxaaConstants : register(b1) // PostProcess.cpp の FxaaConstants と同じ並び
{
    float gEdgeThreshold;    // 輪郭とみなす輝度の差の割合（周りで一番明るい輝度に対して）
    float gEdgeThresholdMin; // 暗い所で輪郭とみなす差の下限
    float gSubpixel;         // 細かい模様をぼかす量（0 で無し、1 で一番強い）
    float gUnused;
};

// 輪郭に沿って探す回数と、回ごとの歩幅（画素。FXAA 3.11 の FXAA_QUALITY__PRESET 12 と同じ。先へ行くほど広げて、長い輪郭も 5 回で探す）
static const int kSearchSteps = 5;
static const float kSearchStride[kSearchSteps] = { 1.0, 1.5, 2.0, 4.0, 12.0 };
static const float kEndGradientRatio = 0.25;	// 輪郭の端とみなす、輝度の離れ方（真ん中と隣の差のうち大きい方の、この割合）
// 周りの平均の重み（上下左右 2・斜め 1 の合計 12 で割る）
static const float kNeighborWeight = 2.0;
static const float kNeighborTotal = 12.0;

float lumaAt(float2 uv)
{
    return gInput.SampleLevel(gClampSampler, uv, 0).a;
}

float lumaAt(float2 uv, int2 offset)
{
    return gInput.SampleLevel(gClampSampler, uv, 0, offset).a;
}

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex v) : SV_Target
{
    uint width, height;
    gInput.GetDimensions(width, height);
    float2 texel = 1.0 / float2(width, height);
    float2 uv = v.uv;
    float4 center = gInput.SampleLevel(gClampSampler, uv, 0);

    // 1) 上下左右との差。小さければ輪郭ではない（暗い所は gEdgeThresholdMin、明るい所は明るさに比例した差で見る）
    float lumaCenter = center.a;
    float lumaUp = lumaAt(uv, int2(0, -1));
    float lumaDown = lumaAt(uv, int2(0, 1));
    float lumaLeft = lumaAt(uv, int2(-1, 0));
    float lumaRight = lumaAt(uv, int2(1, 0));
    float lumaMin = min(lumaCenter, min(min(lumaUp, lumaDown), min(lumaLeft, lumaRight)));
    float lumaMax = max(lumaCenter, max(max(lumaUp, lumaDown), max(lumaLeft, lumaRight)));
    float range = lumaMax - lumaMin;
    float3 result = center.rgb;
    [branch]
    if (range >= max(gEdgeThresholdMin, lumaMax * gEdgeThreshold))
    {
        // 2) 斜めも読み、横向きの輪郭（上下で明るさが変わる）か縦向きの輪郭かを、2 回差分の大きさで決める
        float lumaUpLeft = lumaAt(uv, int2(-1, -1));
        float lumaUpRight = lumaAt(uv, int2(1, -1));
        float lumaDownLeft = lumaAt(uv, int2(-1, 1));
        float lumaDownRight = lumaAt(uv, int2(1, 1));
        float lumaUpDown = lumaUp + lumaDown;
        float lumaLeftRight = lumaLeft + lumaRight;
        float lumaLeftCorners = lumaUpLeft + lumaDownLeft;
        float lumaRightCorners = lumaUpRight + lumaDownRight;
        float lumaUpCorners = lumaUpLeft + lumaUpRight;
        float lumaDownCorners = lumaDownLeft + lumaDownRight;
        float edgeHorizontal = abs(lumaLeftCorners - 2.0 * lumaLeft) + 2.0 * abs(lumaUpDown - 2.0 * lumaCenter)
                             + abs(lumaRightCorners - 2.0 * lumaRight);
        float edgeVertical = abs(lumaUpCorners - 2.0 * lumaUp) + 2.0 * abs(lumaLeftRight - 2.0 * lumaCenter)
                           + abs(lumaDownCorners - 2.0 * lumaDown);
        bool isHorizontal = edgeHorizontal >= edgeVertical;

        // 輪郭を横切る向きで、明るさが大きく変わる側（−側：上か左／＋側：下か右）を選ぶ
        float lumaNegative = isHorizontal ? lumaUp : lumaLeft;
        float lumaPositive = isHorizontal ? lumaDown : lumaRight;
        float gradientNegative = abs(lumaNegative - lumaCenter);
        float gradientPositive = abs(lumaPositive - lumaCenter);
        bool isNegativeSteeper = gradientNegative >= gradientPositive;
        float endGradient = kEndGradientRatio * max(gradientNegative, gradientPositive);
        float crossStep = isHorizontal ? texel.y : texel.x;
        crossStep = isNegativeSteeper ? -crossStep : crossStep;
        float lumaEdgeAverage = 0.5 * (lumaCenter + (isNegativeSteeper ? lumaNegative : lumaPositive));

        // 3) 輪郭の上（真ん中とその隣の境目）に乗り、輪郭に沿って両方向へ端を探す
        float2 edgeUv = uv;
        if (isHorizontal) { edgeUv.y += 0.5 * crossStep; } else { edgeUv.x += 0.5 * crossStep; }
        float2 alongStep = isHorizontal ? float2(texel.x, 0.0) : float2(0.0, texel.y);
        float2 uvBackward = edgeUv - alongStep * kSearchStride[0];
        float2 uvForward = edgeUv + alongStep * kSearchStride[0];
        float endBackward = 0.0;
        float endForward = 0.0;
        bool reachedBackward = false;
        bool reachedForward = false;
        [unroll]
        for (int i = 0; i < kSearchSteps; ++i)
        {
            if (!reachedBackward) { endBackward = lumaAt(uvBackward) - lumaEdgeAverage; }
            if (!reachedForward) { endForward = lumaAt(uvForward) - lumaEdgeAverage; }
            reachedBackward = reachedBackward || abs(endBackward) >= endGradient;
            reachedForward = reachedForward || abs(endForward) >= endGradient;
            if (reachedBackward && reachedForward) { break; }
            if (i + 1 < kSearchSteps)
            {
                if (!reachedBackward) { uvBackward -= alongStep * kSearchStride[i + 1]; }
                if (!reachedForward) { uvForward += alongStep * kSearchStride[i + 1]; }
            }
        }

        // 4) 近い方の端までの距離から、ずらす量（0〜0.5 画素。端に近いほど大きい）
        //    近い方の端の明るさの向き（輪郭の上の平均より暗いか明るいか）が真ん中と同じなら、ギザギザの段はその端の側に無いので、ずらさない
        //    （ずらすと、段の無い所までぼける）
        float distanceBackward = isHorizontal ? (uv.x - uvBackward.x) : (uv.y - uvBackward.y);
        float distanceForward = isHorizontal ? (uvForward.x - uv.x) : (uvForward.y - uv.y);
        bool isBackwardNearer = distanceBackward < distanceForward;
        float nearestDistance = min(distanceBackward, distanceForward);
        float edgeLength = distanceBackward + distanceForward;
        float edgeOffset = 0.5 - nearestDistance / edgeLength;
        bool isCenterDarker = lumaCenter < lumaEdgeAverage;
        bool isEndDarker = (isBackwardNearer ? endBackward : endForward) < 0.0;
        edgeOffset = (isEndDarker != isCenterDarker) ? edgeOffset : 0.0;

        // 5) 細かい模様：周りの平均と真ん中の差が、上下左右の差の幅に対してどれだけ大きいか（なめらかな曲線で強める）
        float lumaAverage = (kNeighborWeight * (lumaUpDown + lumaLeftRight) + lumaLeftCorners + lumaRightCorners) / kNeighborTotal;
        float subpixelRatio = saturate(abs(lumaAverage - lumaCenter) / range);
        float subpixelCurve = (-2.0 * subpixelRatio + 3.0) * subpixelRatio * subpixelRatio; // smoothstep と同じ形
        float subpixelOffset = subpixelCurve * subpixelCurve * gSubpixel;
        float offset = max(edgeOffset, subpixelOffset);

        // 6) 輪郭を横切る向きにずらして線形で読む
        float2 finalUv = uv;
        if (isHorizontal) { finalUv.y += offset * crossStep; } else { finalUv.x += offset * crossStep; }
        result = gInput.SampleLevel(gClampSampler, finalUv, 0).rgb;
    }
    return float4(result, 1.0);
}
