// 半透明の板（弾痕・着弾の砂ぼこり・血しぶき）を、壁や地面と同じライトの一覧（LightList.hlsli）で照らす（LitQuadBatch の MODE_LIT）。
// PbrNightLighting_Plan.txt 10 章。板は粗さ 1・金属 0 の面として照らす（粉や穴は光らないため。段階 2 まではツヤ 0 だった）。
// 色と不透明度は「頂点の色 × 材質の色（明るさの倍率） × 絵」。アルファで後ろと混ぜるのは LitQuadBatch が決める（BLEND_LINEAR）。
// 頂点の計算は StaticMeshGBuffer.hlsl と同じ（固定パイプラインの「ピクセルごとのライティング」と同じ）。
// 板は霧のパス（Fog.hlsl）の後に描くので、照らした色に自分で同じ式の霧を掛ける（遠くの壁の弾痕だけが霧の中で黒く浮かないように。
// NightFinish_Plan.txt 5-4）。
#include "Pbr.hlsli"
#include "Fog.hlsli"

static const float kRoughSurface = 1.0;
static const float kNotMetal = 0.0;
static const float kNoOcclusion = 1.0;

struct PixelInput
{
    float4 position      : SV_Position;
    float4 color         : COLOR0;
    float2 uv            : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
    float3 worldNormal   : TEXCOORD2;
};

PixelInput VSMain(StandardVertex v)
{
    PixelInput o;
    o.position = toClip(v.position);
    o.worldPosition = toWorld(v.position);
    o.worldNormal = normalToWorld(v.normal);
    o.color = vertexColor(v.color) * gDiffuseColor;
    o.uv = v.uv;
    return o;
}

float4 PSMain(PixelInput p) : SV_Target
{
    float4 c = p.color * gAlbedo.Sample(gWrapSampler, p.uv);
    Surface s;
    s.position = p.worldPosition;
    s.normal = normalize(p.worldNormal);
    s.albedo = c.rgb;
    s.roughness = kRoughSurface;
    s.metallic = kNotMetal;
    s.occlusion = kNoOcclusion;
    return float4(applyFog(shadeSurface(s), s.position), c.a);
}
