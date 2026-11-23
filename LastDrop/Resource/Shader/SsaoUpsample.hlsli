// SSAO の結果（AmbientOcclusion。画面の半分の大きさ・r に AO・g に目からの奥行き m）を読む側の共通の式
// ぼかし（SsaoBlur.hlsl）と、元の大きさへの拡大（DeferredLighting.hlsl のライティングと表示の SSAO）が同じ「奥行きの近さ」を使う
#ifndef INCLUDED_SSAO_UPSAMPLE_HLSLI
#define INCLUDED_SSAO_UPSAMPLE_HLSLI

// 奥行きの近さで許す差（真ん中の奥行きに対する割合）。遠い物ほど許す差を大きくする（同じ壁でも遠くは 1 画素で奥行きが大きく変わるため）
// 画面で見て決めた値ではない初期値。輪郭の外に縁が出るなら下げ、平らな地面にまだらが残るなら上げる
static const float kSsaoDepthTolerance = 0.05;
static const float kSsaoMinWeight = 1e-4;     // 重みの和がこれより小さいときは、奥行きの一番近い画素だけを使う
static const float kSsaoHalfStep = 0.5;       // 元の画面の 1 画素 = 半分の大きさの 0.5 画素（Ssao.hlsl の kHalfResolutionStep の逆）

// 奥行き d の画素を、奥行き center の画素とどれだけ混ぜてよいか（0〜1。同じ奥行きで 1）
float ssaoDepthWeight(float d, float center)
{
    return exp(-abs(d - center) / (center * kSsaoDepthTolerance));
}

// 元の画面の画素 pixel（奥行き viewDepth）の AO を、半分の大きさの結果から読む
// 近くの 4 画素を、双線形の重み × 奥行きの近さで混ぜる（ただの双線形だと、輪郭で奥と手前の AO が混ざって 1 画素の縁が出るため）
// 半分の大きさの画素 i は、元の画面の画素 2i の値（Ssao.hlsl。2×2 の左上で代表）
float upsampleOcclusion(Texture2D occlusion, uint2 pixel, float viewDepth)
{
    uint width, height;
    occlusion.GetDimensions(width, height);
    int2 last = int2(width, height) - 1;
    float2 h = pixel * kSsaoHalfStep;
    int2 base = (int2)floor(h);
    float2 f = h - base;
    float weightSum = 0.0;
    float sum = 0.0;
    float nearestDifference = 1e30;
    float nearest = 1.0;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        int2 offset = int2(i & 1, i >> 1);
        float2 texel = occlusion.Load(int3(clamp(base + offset, int2(0, 0), last), 0)).rg;
        float bilinear = (offset.x ? f.x : 1.0 - f.x) * (offset.y ? f.y : 1.0 - f.y);
        float weight = bilinear * ssaoDepthWeight(texel.g, viewDepth);
        weightSum += weight;
        sum += texel.r * weight;
        float difference = abs(texel.g - viewDepth);
        if (difference < nearestDifference)
        {
            nearestDifference = difference;
            nearest = texel.r;
        }
    }
    return (weightSum > kSsaoMinWeight) ? sum / weightSum : nearest;
}

#endif
