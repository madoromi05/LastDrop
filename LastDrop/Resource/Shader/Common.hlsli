// 自作シェーダの共通部分。番号の決まりは GameLib/Graphics/Manager.h の enum と揃える
//   b0     : 固定パイプラインと同じ定数（頂点シェーダから読む。下の VSConstants）
//   b1〜   : 自作の定数（Manager::setConstantBuffer）
//   t0, t1 : setTexture / setNormalMap
//   t2〜   : 自作のテクスチャ・描画先（Manager::setShaderResource）
//   s0     : 繰り返し（setTextureFilter で点/線形が決まる）  s1 : 端で止める線形  s2 : 影の比較
#ifndef INCLUDED_COMMON_HLSLI
#define INCLUDED_COMMON_HLSLI

// b0（頂点シェーダ側）。GameLib/src/Graphics/ManagerImpl.h の VSCBufferData と同じ並び
// 行列は行ごとの float4 で入っているので、mul ではなく行との内積で掛ける（toClip など）
cbuffer VSConstants : register(b0)
{
    float4 gTransform[4];     // 透視変換 * ビュー * ワールド
    float4 gWorldMatrix[3];   // ワールド（3x4）
    float4 gITWorldMatrix[3]; // ワールドの逆行列の転置（法線用）
    float4 gDiffuseColor;
    float4 gAmbientColor;
    float4 gLightPosX;
    float4 gLightPosY;
    float4 gLightPosZ;
    float4 gLightColor[4];
    float4 gColorParams;      // x: 1 なら頂点色をリニアへ直す（Manager::enableLinearColor。色の定数は GameLib が直して入れる）
};

// 画像の色（sRGB）と、光の量に比例した数（リニア）の変換。式は GameLib/src/Graphics/ColorSpace.h・ColorSpaceHlsl.h と同じ
// （pow 2.2 の近似ではなく、GPU が _SRGB のテクスチャを読むときと同じ式）
float3 srgbToLinear(float3 c)
{
    return (c <= 0.04045) ? (c / 12.92) : pow(abs((c + 0.055) / 1.055), 2.4);
}
float3 linearToSrgb(float3 c)
{
    c = saturate(c);
    return (c <= 0.0031308) ? (c * 12.92) : (1.055 * pow(c, 1.0 / 2.4) - 0.055);
}

SamplerState           gWrapSampler   : register(s0);
SamplerState           gClampSampler  : register(s1);
SamplerComparisonState gShadowSampler : register(s2);

// 標準の頂点（VertexFormat::standard）
struct StandardVertex
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};

float4 toClip(float4 position)
{
    return float4(dot(gTransform[0], position), dot(gTransform[1], position),
                  dot(gTransform[2], position), dot(gTransform[3], position));
}
float3 toWorld(float4 position)
{
    return float3(dot(gWorldMatrix[0], position), dot(gWorldMatrix[1], position), dot(gWorldMatrix[2], position));
}
float3 normalToWorld(float3 normal)
{
    return float3(dot(gITWorldMatrix[0].xyz, normal), dot(gITWorldMatrix[1].xyz, normal), dot(gITWorldMatrix[2].xyz, normal));
}
// 頂点色は固定パイプラインと同じく .zyxw で並べ替えて使う（GameLib の頂点色の詰め方に合わせる）
// 3D をリニアで描く間は、色（アルファ以外）をリニアへ直す（ColorSpaceHlsl.h の vertexColorToScene と同じ）
float4 vertexColor(float4 color)
{
    float4 c = color.zyxw;
    if (gColorParams.x > 0.5)
    {
        c.rgb = srgbToLinear(c.rgb);
    }
    return c;
}

// 全画面描画（Manager::drawFullScreen）の頂点。画面を覆う大きな三角形1枚を SV_VertexID（0,1,2）から作る
// uv は画面の左上が (0,0)、右下が (1,1)（テクスチャと同じ向き）
struct FullScreenVertex
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};
FullScreenVertex fullScreenVertex(uint id)
{
    FullScreenVertex o;
    o.uv = float2((id << 1) & 2, id & 2);
    o.position = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}

#endif
