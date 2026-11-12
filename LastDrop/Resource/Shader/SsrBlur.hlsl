// 映り込み（SSR）のぼかし（ScreenSpaceReflection::draw。SsrDecalMaterial_Plan.txt 7-4）。横 → 縦の 2 回、半分の大きさの描画先どうしで描く
// 歩き始めを画素ごとにずらした（Ssr.hlsl）ざらつきを均す。重みはガウス × 奥行きの近さ（SSAO のぼかしと同じ考え）
// SSR の結果は奥行きを持たないので、奥行きはシーンの深度から読む（半分の大きさの画素 i は元の画面の画素 2i）
// 色は確からしさを掛けた物（SsrCommon.hlsli）なので、そのまま 4 つとも同じ重みで混ぜてよい
// 向きをばらした粗い面は、真ん中の粗さで読む間隔を広げ（0.4 以下は 1 画素・0.7 で 3 画素）、
// 粗さが大きく違う画素は混ぜない（つやの違う面どうしの映り込みがにじみ合わないように）
#include "Common.hlsli"
#include "SsrCommon.hlsli"

Texture2D        gSource          : register(t2); // ScreenSpaceReflection.cpp の kBlurSourceSlot（半分の大きさ）
Texture2D        gGBufferMaterial : register(t4); // 同 kMaterialSlot（G-Buffer の材質。g が粗さ。元の大きさ）
Texture2D<float> gSceneDepth      : register(t5); // 同 kDepthSlot（シーンの深度。元の大きさ）

// ScreenSpaceReflection.cpp の SsrBlurConstants と同じ並び
cbuffer SsrBlurConstants : register(b1)
{
    float4 gBlurInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）
    float4 gBlurViewDepthRow;             // 射影×ビューの 4 行目（位置との内積が奥行き）
    float4 gBlurDirection;                // xy: 1 画素ずつ進む向き（横 (1,0)・縦 (0,1)）  z: 粗さで読む間隔を広げるか（1 / 0）  w: 使わない
};

static const int kBlurRadius = 4;     // 真ん中から片側に読む画素の数（SsaoBlur.hlsl と同じ）
static const float kBlurSigma = 2.0;  // ガウスの広がり（画素）
static const float kRoughSpacingExtra = 2.0;   // 粗さ kSsrSharpRoughness + kSsrRoughRange で読む間隔に足す画素（1 → 3）
static const float kMaxRoughnessDifference = 0.2; // 真ん中と粗さの差がこれより大きい画素は混ぜない

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    uint width, height;
    gSource.GetDimensions(width, height);
    uint depthWidth, depthHeight;
    gSceneDepth.GetDimensions(depthWidth, depthHeight);
    uint2 depthSize = uint2(depthWidth, depthHeight);
    int2 last = int2(width, height) - 1;
    int2 pixel = (int2)p.position.xy;
    int2 direction = (int2)gBlurDirection.xy;
    float centerDepth = ssrViewDepthAt(gSceneDepth, gBlurInverseProjectionView, gBlurViewDepthRow, pixel * kSsrHalfStep, depthSize);
    bool roughAware = gBlurDirection.z > 0.5;
    float centerRoughness = gGBufferMaterial.Load(int3(min(pixel * kSsrHalfStep, int2(depthSize) - 1), 0)).g;
    float spacing = roughAware ? 1.0 + kRoughSpacingExtra * ssrRoughBlend(centerRoughness) : 1.0;
    float weightSum = 0.0;
    float4 sum = 0.0;
    [unroll]
    for (int i = -kBlurRadius; i <= kBlurRadius; ++i)
    {
        int2 neighbor = clamp(pixel + direction * (int)round(i * spacing), int2(0, 0), last);
        int2 neighborFull = min(neighbor * kSsrHalfStep, int2(depthSize) - 1);
        float neighborDepth = ssrViewDepthAt(gSceneDepth, gBlurInverseProjectionView, gBlurViewDepthRow, neighborFull, depthSize);
        float weight = exp(-(i * i) / (2.0 * kBlurSigma * kBlurSigma)) * ssaoDepthWeight(neighborDepth, centerDepth);
        if (roughAware && abs(gGBufferMaterial.Load(int3(neighborFull, 0)).g - centerRoughness) > kMaxRoughnessDifference)
        {
            weight = 0.0; // 真ん中（i = 0）は差が 0 なので、weightSum は 1 以上のまま
        }
        weightSum += weight;
        sum += gSource.Load(int3(neighbor, 0)) * weight;
    }
    // 真ん中の重みは 1（ガウスも奥行きの近さも 1）なので、weightSum は 1 以上
    return sum / weightSum;
}
