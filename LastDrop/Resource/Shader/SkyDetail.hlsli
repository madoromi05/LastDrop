// 空に足す月の絵（MoonStarsLut_Plan.txt 4 章。星は 2026-10-31 に削除）。LightList.hlsli の skyColor・environmentLight から呼ぶ。
// 値はライトの一覧の定数（gMoonDirection・gMoonColor・gMoonHalo。LightList.cpp）。明るさ 0 なら何も足さない。
// 町を照らす月の光（ライトの一覧の平行光）とは別：ここは見える絵だけで、地面の明るさは変えない。
#ifndef INCLUDED_SKY_DETAIL_HLSLI
#define INCLUDED_SKY_DETAIL_HLSLI

// 月の模様（Resource/Texture/Map/moon.dds。tools/moon_gen.py。rgb 面の明るさ・a 月の形）。DeferredLighting が空を描くパス（drawLighting）でだけ t14 に付ける。
Texture2D gMoonTexture : register(t14);

// ── 月 ──
// 模様を読む範囲（月の半径の何倍まで）。縁を 1 画素なめらかにした分だけ、半径の少し外まで読む。
static const float kMoonTextureReach = 1.1;
// 月の模様の画像の、半径にあたる画素の数の割合（moon_gen.py は画像の半分から縁の 2 画素を引いた所を半径にしている）。
static const float kMoonTextureRadiusRatio = 0.5;
// 水たまりなどに映す月は、粗さがこれより上で消える（ざらざらの面に小さな円板がくっきり映らないように）。
static const float kMoonReflectionMaxRoughness = 0.5;
// 映す月の円板の縁をなめらかにする幅（半径に対する割合。映り込みの画素の大きさは分からないので一定の幅にする）。
static const float kMoonReflectionEdge = 0.15;

// 月の方への向きに垂直な 2 本の軸（右・上）。真上・真下に月があるときは横の軸を別に取る。
void moonAxes(out float3 right, out float3 up)
{
    float3 toMoon = gMoonDirection.xyz;
    float3 reference = (abs(toMoon.y) < 0.99) ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    right = normalize(cross(reference, toMoon));
    up = cross(toMoon, right);
}

// 向き dir と月の中心の角度を、月の半径で割った数（0 が中心・1 が縁）。
float moonDistanceRatio(float3 dir)
{
    float angle = acos(clamp(dot(dir, gMoonDirection.xyz), -1.0, 1.0));
    return angle / gMoonDirection.w;
}

// 月の色（リニア。円板の明るさ込み）。
float3 moonRadiance()
{
    return srgbToLinear(gMoonColor.rgb) * gMoonColor.w;
}

// 空に描く月（模様つきの円板＋周りのにじみ）。gMoonColor.w が 0 なら 0。
float3 moonWithTexture(float3 dir)
{
    if (gMoonColor.w <= 0.0)
    {
        return 0.0;
    }
    float ratio = moonDistanceRatio(dir);
    float3 radiance = moonRadiance();
    float3 result = radiance * gMoonHalo.x * exp(-ratio / gMoonHalo.y);
    if (ratio < kMoonTextureReach)
    {
        float3 right;
        float3 up;
        moonAxes(right, up);
        // 月に垂直な面へ写した位置は sin(角度)。半径の sin で割ると、円板の中が −1〜1 になる。
        float2 disc = float2(dot(dir, right), dot(dir, up)) / sin(gMoonDirection.w);
        float2 uv = float2(disc.x, -disc.y) * 0.5 + 0.5;
        // 読む段（ミップ）は、画面の 1 画素に模様の何画素が入るかで決める（全画面のパスの分かれ道の中なので、自動の段の選び方に頼らない）。
        float width;
        float height;
        gMoonTexture.GetDimensions(width, height);
        float texelsPerPixel = width * kMoonTextureRadiusRatio * gMoonHalo.z / gMoonDirection.w;
        float4 surface = gMoonTexture.SampleLevel(gClampSampler, uv, log2(max(texelsPerPixel, 1.0)));
        result += radiance * surface.rgb * surface.a;
    }
    return result;
}

// つるつるの面に映す月（模様なし・にじみなし）。粗いほど薄くし、kMoonReflectionMaxRoughness で消える。
float3 reflectedMoon(float3 dir, float roughness)
{
    float weight = saturate(1.0 - roughness / kMoonReflectionMaxRoughness);
    if (gMoonColor.w <= 0.0 || weight <= 0.0)
    {
        return 0.0;
    }
    float disc = 1.0 - smoothstep(1.0 - kMoonReflectionEdge, 1.0, moonDistanceRatio(dir));
    return moonRadiance() * disc * weight;
}

#endif
