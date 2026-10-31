// PBR の式（SummaryPlan/個別仕様/PbrNightLighting_Plan.txt 4 章）。ディファードのライティングのパス（DeferredLighting.hlsl）と半透明の板（LitQuad.hlsl）が使う。
//   拡散反射：Lambert   鏡面反射：Cook-Torrance（D：GGX  G：Smith の Schlick-GGX 近似  F：Schlick）
//   材質：メタリック・ラフネス（粗さ・金属。Blender・glTF・Unreal と同じ）
// 式は「面の BRDF（surfaceBrdf）」と「ライトの向きと届く光（evaluateLight。種類は LightList.hlsli）」に分ける。
// ライトの種類を増やすときは、ライトの側だけ足す（BRDF は 1 か所のまま）。
//
// 光の強さの単位：ライトの値は「π を掛け済み」として扱う（拡散を albedo / π ではなく albedo で計算し、鏡面には π を掛ける）。
// こうすると拡散の明るさが段階 2 までの式（albedo × ライトの色 × N・L）と同じになり、3-a で変わるのはツヤの出方だけになる。
#ifndef INCLUDED_PBR_HLSLI
#define INCLUDED_PBR_HLSLI

#include "Lighting.hlsli"
#include "LightList.hlsli"

static const float kPi = 3.14159265;
static const float3 kDielectricF0 = float3(0.04, 0.04, 0.04); // 金属でない物の正面の反射率（4%。水・プラスチック・石などの目安）
// 粗さの下限。0 に近いと点光源のハイライトが 1 画素より小さくなり、ちらつく（Karis の資料でも下限を置く）。
static const float kMinRoughness = 0.045;
// N・V に足す小さな数。真横から見た面で 0 割りにならないように（見た目に効く大きさではない）。
static const float kNoVBias = 1e-5;
// 点光源・スポットの距離の下限（10cm）。距離の 2 乗で弱まるので、ライトのすぐ近くの面は何百倍にも明るくなる。
// マズルフラッシュの光は銃口の 5cm 前にあり、銃そのものが真っ白にならないよう、これより近くは 10cm と同じ明るさにする。
static const float kMinLightDistanceSquared = 0.01;
// Smith の Schlick-GGX 近似で、点光源用の k ＝ (粗さ + 1)² / 8（Karis「Real Shading in Unreal Engine 4」）。
static const float kSmithRemapOffset = 1.0;
static const float kSmithRemapDivisor = 8.0;
static const float kSchlickPower = 5.0;
// Cook-Torrance の分母（4 × N・L × N・V）の 4。
static const float kCookTorranceDenominator = 4.0;
// 拡散の環境光は、法線の側の半球全体をならした色（粗さ 1 と同じ引き方）。
static const float kFullyRough = 1.0;

// 1 画素の面の情報（G-Buffer から読んだ物）。
struct Surface
{
    float3 position;
    float3 normal;    // 長さ 1
    float3 albedo;    // リニア
    float roughness;
    float metallic;
    float occlusion;  // AO（1 で遮られていない）
};

// 面の色から、拡散反射の色（金属は拡散しない）と正面の反射率 F0（金属は自分の色で映す）を作る。
float3 diffuseColorOf(Surface s)
{
    return s.albedo * (1.0 - s.metallic);
}
float3 f0Of(Surface s)
{
    return lerp(kDielectricF0, s.albedo, s.metallic);
}

// N・V（面から目への向き v と法線の内積）。補間した法線・法線マップでは、輪郭の近くで法線が目と反対を向く（内積が負の）画素ができる。
// 負を下限で止めると「真横から見た」扱いになり、フレネルで鏡面が何十倍にもなって逆光の輪郭が光る（2026-10-31 ゾンビの体）。
// 負は向きを裏返した値で計算する（Lagarde「Moving Frostbite to PBR」と同じ abs）。
float viewCosine(float3 n, float3 v)
{
    return abs(dot(n, v)) + kNoVBias;
}

float distributionGgx(float noh, float alpha)
{
    float a2 = alpha * alpha;
    float d = noh * noh * (a2 - 1.0) + 1.0;
    return a2 / (kPi * d * d);
}

float geometrySchlickGgx(float nox, float k)
{
    return nox / (nox * (1.0 - k) + k);
}

float geometrySmith(float nol, float nov, float roughness)
{
    float r = roughness + kSmithRemapOffset;
    float k = r * r / kSmithRemapDivisor;
    return geometrySchlickGgx(nol, k) * geometrySchlickGgx(nov, k);
}

float3 fresnelSchlick(float3 f0, float voh)
{
    return f0 + (1.0 - f0) * pow(1.0 - voh, kSchlickPower);
}

// 1 つのライトから届く光 radiance が、向き l（面からライトへ・長さ 1）で当たったときに、目の向き v へ返る色。
float3 surfaceBrdf(Surface s, float3 l, float3 v, float3 radiance)
{
    float nol = saturate(dot(s.normal, l));
    if (nol <= 0.0)
    {
        return 0.0;
    }
    float roughness = max(s.roughness, kMinRoughness);
    float alpha = roughness * roughness;
    float3 h = normalize(l + v);
    float nov = viewCosine(s.normal, v);
    float noh = saturate(dot(s.normal, h));
    float voh = saturate(dot(v, h));
    float3 f = fresnelSchlick(f0Of(s), voh);
    float3 specular = distributionGgx(noh, alpha) * geometrySmith(nol, nov, roughness) * f
                    / (kCookTorranceDenominator * nol * nov);
    // ライトは π を掛け済み（上の説明）なので、拡散は albedo のまま、鏡面に π を掛ける。
    return (diffuseColorOf(s) + kPi * specular) * radiance * nol;
}

// 空と地面に囲まれたときの色（拡散＋鏡面。LightList.hlsli の environmentLight）。
//   拡散：法線の向きの半球ライト（上を向く面ほど空の色）
//   鏡面：反射の向きに見える空・地面の色を、粗さでならす（キューブマップは作らない。Graphics_Spec.txt 8 章）。
//         映り込みの強さは Karis の近似（EnvBRDFApprox。「Physically Based Shading on Mobile」）で、反射の積分を粗さと N・V の式で置き換える。
// 金属は拡散しないので、これが無いと環境光だけの所で真っ黒になる。
// 映り込み（SSR。SsrComposite.hlsl）は、鏡面の「空と地面の色」を画面に映っている物の色へ入れ替える（SsrDecalMaterial_Plan.txt 7-3）。
// 映り込みの強さ（environmentSpecularWeight）はここと SSR で同じ式を使う。

// 映り込みの強さ（Karis の EnvBRDFApprox）。v：面から目への向き（長さ 1）。
float3 environmentSpecularWeight(Surface s, float3 v)
{
    static const float4 kEnvC0 = float4(-1.0, -0.0275, -0.572, 0.022);
    static const float4 kEnvC1 = float4(1.0, 0.0425, 1.04, -0.04);
    static const float kEnvFalloff = -9.28;
    static const float2 kEnvScaleBias = float2(-1.04, 1.04);
    float nov = viewCosine(s.normal, v);
    float4 r = s.roughness * kEnvC0 + kEnvC1;
    float a004 = min(r.x * r.x, exp2(kEnvFalloff * nov)) * r.x + r.y;
    float2 ab = kEnvScaleBias * a004 + r.zw;
    return f0Of(s) * ab.x + ab.y;
}

float3 ambientLighting(Surface s, float3 v)
{
    float3 specularWeight = environmentSpecularWeight(s, v);
    float3 reflected = reflect(-v, s.normal);
    float3 diffuse = diffuseColorOf(s) * environmentLight(s.normal, kFullyRough);
    float3 specular = specularWeight * environmentLight(reflected, s.roughness);
    return (diffuse + specular) * s.occlusion;
}

// ライト 1 つ（LightList.hlsli）が面に当たったときに、目の向き v へ返る色（PbrNightLighting_Plan.txt 6 章）。
//   平行光：向きだけ。減衰しない
//   点光源：距離の 2 乗に反比例 × 届く距離でなめらかに 0（窓 = saturate( 1 − (d / range)⁴ )²。UE4・Frostbite と同じ形）
//   スポット：点光源の減衰 × 中心からの角度でなめらかに暗く（saturate( cos × 倍率 + ずらし )²。Frostbite と同じ形）
//   影のマップを持つライト（懐中電灯・月）は、さらに影の分だけ暗く（LightList.hlsli の shadowVisibility）
float3 evaluateLight(Surface s, float3 v, LightData light)
{
    float3 radiance = srgbToLinear(light.colorIntensity.rgb) * light.colorIntensity.w; // 色だけ直してから強さを掛ける（GameLib と同じ）
    uint kind = (uint)light.directionKind.w;
    // 平行光の値で始め、点光源・スポットなら向きと減衰を置き換える（return を 1 か所にする。途中で返すと fxc が
    // 「初期化していないかもしれない」と警告するため）。
    float3 l = -light.directionKind.xyz;
    float attenuation = 1.0;
    float lightDistance = 1.0; // 平行光では使わない（影を読む位置のずらしは、正射影なので距離に関わらず同じ）
    if (kind != kLightDirectional)
    {
        float3 toLight = light.positionRange.xyz - s.position;
        float distanceSquared = max(dot(toLight, toLight), kMinLightDistanceSquared);
        float rcpDistance = rsqrt(distanceSquared);
        l = toLight * rcpDistance;
        lightDistance = distanceSquared * rcpDistance;
        float rangeRatio = distanceSquared / (light.positionRange.w * light.positionRange.w); // (d / range)²
        float window = saturate(1.0 - rangeRatio * rangeRatio);
        attenuation = window * window / distanceSquared;
        if (kind == kLightSpot)
        {
            float cone = saturate(dot(-l, light.directionKind.xyz) * light.spotScaleOffset.x + light.spotScaleOffset.y);
            attenuation *= cone * cone;
        }
    }
    // 影（Shadow_Plan.txt 4 章）。影のマップを持つライトだけ、光が届いている所で影のマップを読む。
    // 影のマップが 2 枚以上のライト（月の CSM）は、カメラからの奥行きで枚を選ぶ（6 章）。
    int shadowMap = (int)light.spotScaleOffset.z;
    int shadowMapCount = (int)light.spotScaleOffset.w;
    if (shadowMap >= 0 && attenuation > 0.0)
    {
        attenuation *= (shadowMapCount > 1)
            ? cascadedShadowVisibility((uint)shadowMap, s.position, s.normal, l, gEyePosition.xyz)
            : shadowVisibility((uint)shadowMap, s.position, s.normal, l, lightDistance);
    }
    return surfaceBrdf(s, l, v, radiance * attenuation);
}

// 面を、環境光とライトの一覧（b2）で照らした色（発光は含まない）。
float3 shadeSurface(Surface s)
{
    float3 v = normalize(gEyePosition.xyz - s.position);
    float3 c = ambientLighting(s, v);
    uint count = min((uint)gLightCount.x, kMaxLights);
    for (uint i = 0; i < count; ++i)
    {
        c += evaluateLight(s, v, gLights[i]);
    }
    return c;
}

#endif
