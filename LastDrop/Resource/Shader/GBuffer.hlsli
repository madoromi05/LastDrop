// G-Buffer の並び（SummaryPlan/個別仕様/グラフィック/GBuffer_Plan.txt 4 章・PbrNightLighting_Plan.txt 5 章）。書く側（StaticMeshGBuffer.hlsl・
// SkinnedMeshGBuffer.hlsl）と読む側（DeferredLighting.hlsl）が、並びをここだけで知る。並びを変えるときは、ここと GBuffer.cpp の形式を一緒に直す。
//   t2 / RT0  RGBA8_SRGB : rgb 色（頂点色×材質の色×画像。リニアで書くと GPU が sRGB にして入れる）  a 予備
//   t3 / RT1  RGBA16F    : xyz 法線（ワールド・長さ 1・法線マップ済み）                          a 動きの種類（0 町・0.5 ゾンビ・1 手元。TAA）
//   t4 / RT2  RGBA8      : r AO（材質の画像の r。無ければ 1）  g 粗さ  b 金属  a 発光の強さ（0〜1。色は「色 × 強さ × kEmissionScale」）
//   t5        深度       : PostProcess の深度（位置は深度から戻す）
#ifndef INCLUDED_GBUFFER_HLSLI
#define INCLUDED_GBUFFER_HLSLI

#include "Lighting.hlsli"

static const float kSpare = 0.0;     // 予備のチャンネルに書く値
// 発光の強さ 1 のときの明るさ（色に掛ける）。HDR なので 1 を超えてよい（ブルームで光る）。画面で見て決めた値ではない初期値。
static const float kEmissionScale = 8.0;

// b3：次に描く物の PBR の材質値（MeshMaterial::apply → RenderPass::setSurface。GBuffer.cpp の kSurfaceSlot）。
// b1 はキャラの骨（Skinning.hlsli）、b2 は段階 3-b のライトの一覧に使うので空けておく。
cbuffer GBufferMaterial : register(b3)
{
    float4 gSurface; // x: 粗さ  y: 金属  z: 発光の強さ  w: 水たまりを書くか（0 か 1。道路だけ 1）
};

// 書く側だけが読む物（SsrDecalMaterial_Plan.txt 5・6 章）。番号は読む側（t2〜t5・SSAO の t10）と重ならない所。
// 材質の画像（r AO・g 粗さ・b 金属）。無い材質には白 1 画素が付く（RenderPass::setSurface）ので、数値 × 画像 = 数値のまま。
Texture2D gOrmMap     : register(t11);
// 雨上がりの水たまりの模様（GBuffer.cpp の kPuddleMaskSlot。tools/texgen/puddle_gen.py）。ワールドの x・z で読む。
Texture2D gPuddleMask : register(t12);
// b4：G-Buffer へ描く間ずっと同じ値（GBuffer.cpp の PassConstants）。水たまりの値（PuddleParams から）と、今描いている物の動きの種類。
cbuffer GBufferPassConstants : register(b4)
{
    float4 gPuddleShape;   // x: しきい値（模様の値がこれより大きい所が水たまり）  y: 濡れた縁の幅  z: 1 / 模様 1 枚の大きさ（m）  w: 使わない
    float4 gPuddleSurface; // x: 水たまりの粗さ  y: 濡れた路面の粗さ  z: 濡れた所の色に掛ける数  w: 使わない
    float4 gMotion;        // x: 動きの種類（0 動かない物・0.5 ゾンビなど動く物・1 手元。Taa_Plan.txt 6 章）。RT1 の a に書く  yzw: 使わない
};
static const float kPuddleEdge = 0.02;  // 濡れた路面から水たまりへ変わる幅（模様の値で。狭いほど水面の縁がくっきり）
static const float kPuddleMinUp = 0.9;  // 面の上向きの度合い（法線の y）がこれより小さい面（縁石の側面・壁）には水が溜まらない

// 道路に雨上がりの水たまりを書き込む（SsrDecalMaterial_Plan.txt 6-2）。水たまりの周りは濡れて暗く・つやが出て、
// 水たまりの中は鏡に近いつやで、法線は面の向きそのまま（水面は平ら。凹凸の法線マップを消す）。
void applyPuddle(float3 worldPosition, float3 geometricNormal, inout float3 albedo, inout float3 n, inout float roughness)
{
    float mask = gPuddleMask.Sample(gWrapSampler, worldPosition.xz * gPuddleShape.z).r;
    float level = saturate((geometricNormal.y - kPuddleMinUp) / (1.0 - kPuddleMinUp));
    float wet = smoothstep(gPuddleShape.x - gPuddleShape.y, gPuddleShape.x, mask) * level;
    float puddle = smoothstep(gPuddleShape.x, gPuddleShape.x + kPuddleEdge, mask) * level;
    roughness = lerp(roughness, min(roughness, gPuddleSurface.y), wet);
    roughness = lerp(roughness, gPuddleSurface.x, puddle);
    albedo *= lerp(1.0, gPuddleSurface.z, wet);
    n = normalize(lerp(n, geometricNormal, puddle));
}

struct GBufferOutput
{
    float4 albedo   : SV_Target0;
    float4 normal   : SV_Target1;
    float4 material : SV_Target2;
};

// 1 画素ぶんの面の情報を G-Buffer の並びにする。color: 頂点色×ディフューズ色（リニア）。
// 法線マップは今の描き方（Lighting.hlsli の shadePixel）と同じ方法で掛ける。
// 材質の画像（ORM）は粗さ・金属に掛け、AO はそのまま r へ（ライティングで SSAO と掛け合わせる）。
GBufferOutput writeGBuffer(float4 color, float2 uv, float3 worldPosition, float3 worldNormal, bool isFrontFace)
{
    float3 geometric = normalize(worldNormal);
    geometric = isFrontFace ? geometric : -geometric; // 裏を描いたとき（doublesided="1" の材質）は、見ている側へ法線を向ける
    float3 n = perturbNormal(geometric, worldPosition, uv);
    float3 albedo = (color * gAlbedo.Sample(gWrapSampler, uv)).rgb;
    float3 orm = gOrmMap.Sample(gWrapSampler, uv).rgb;
    float roughness = saturate(gSurface.x * orm.g);
    float metallic = saturate(gSurface.y * orm.b);
    if (gSurface.w > 0.0)
    {
        applyPuddle(worldPosition, geometric, albedo, n, roughness);
    }
    GBufferOutput o;
    o.albedo = float4(albedo, kSpare);
    o.normal = float4(n, gMotion.x); // a は動きの種類（TAA が前のフレームのどこを読むかを分ける）
    o.material = float4(orm.r, roughness, metallic, saturate(gSurface.z));
    return o;
}

// 読む側。番号は DeferredLighting.cpp と揃える。
Texture2D        gGBufferAlbedo   : register(t2);
Texture2D        gGBufferNormal   : register(t3);
Texture2D        gGBufferMaterial : register(t4);
Texture2D<float> gSceneDepth      : register(t5);

#endif
