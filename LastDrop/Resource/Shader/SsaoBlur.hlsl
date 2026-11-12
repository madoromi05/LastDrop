// SSAO のぼかし（AmbientOcclusion::draw。SsaoTranslucent_Plan.txt 6-1・Q3）。横 → 縦の 2 回、半分の大きさの描画先どうしで描く
// 画素ごとに回した点の並び（Ssao.hlsl）のざらつきを均す。重みはガウス（離れた画素ほど小さく）× 奥行きの近さで、
// 奥行きが大きく違う画素（ゾンビの輪郭の外の壁など）は混ぜない（暗がりが輪郭の外へにじまない）
// 出力：r にぼかした AO、g に真ん中の奥行き（ぼかさずそのまま写す。拡大で使う）
#include "Common.hlsli"
#include "SsaoUpsample.hlsli"

Texture2D gSource : register(t2); // AmbientOcclusion.cpp の kBlurSourceSlot（半分の大きさ・r AO・g 奥行き）

cbuffer SsaoBlurConstants : register(b1) // AmbientOcclusion.cpp の SsaoBlurConstants と同じ並び
{
    float4 gBlurDirection; // xy: 1 画素ずつ進む向き（横 (1,0)・縦 (0,1)）  zw: 使わない
};

static const int kBlurRadius = 4;     // 真ん中から片側に読む画素の数
static const float kBlurSigma = 2.0;  // ガウスの広がり（画素）

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    uint width, height;
    gSource.GetDimensions(width, height);
    int2 last = int2(width, height) - 1;
    int2 pixel = (int2)p.position.xy;
    float2 center = gSource.Load(int3(pixel, 0)).rg;
    int2 direction = (int2)gBlurDirection.xy;
    float weightSum = 0.0;
    float sum = 0.0;
    [unroll]
    for (int i = -kBlurRadius; i <= kBlurRadius; ++i)
    {
        float2 texel = gSource.Load(int3(clamp(pixel + direction * i, int2(0, 0), last), 0)).rg;
        float weight = exp(-(i * i) / (2.0 * kBlurSigma * kBlurSigma)) * ssaoDepthWeight(texel.g, center.g);
        weightSum += weight;
        sum += texel.r * weight;
    }
    return float4(sum / weightSum, center.g, 0.0, 1.0);	// 真ん中の重みは 1（ガウスも奥行きの近さも 1）なので、weightSum は 1 以上
}
