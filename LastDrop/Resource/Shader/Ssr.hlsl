// 映り込み（SSR）を、画面の半分の大きさの描画先（RGBA16F）へ作る（ScreenSpaceReflection::draw。SsrDecalMaterial_Plan.txt 7-2）。
// 画素の位置から、目からの向きを法線で反射した向きへ進み、深度より奥へ入った所（物に当たった所）を探して、
// その画素の「照らした後のシーンの色」を持ってくる。進み方は 2 つ（F6 → Render の Hi-Z で切り替える）：
//   Hi-Z（Uludag 2014）… 深度の縮小の段で、何も無い所を大きく飛ばして進む
//   直線 ＋ 二分探索（SsrDecalMaterial_Plan.txt Q5 の A）… 段階12 の計測が終わったら消す
// 粗い面（Stachowiak 2015）は、反射の向きを GGX の見える法線の分布から画素ごとに 1 本選んでばらし、
// ぼかし（SsrBlur.hlsl）と TAA が均す（F6 → Render の Stochastic SSR。切ると鏡の向きだけ）。
// 出力：rgb は「映る色 × 確からしさ」、a は確からしさ（SsrCommon.hlsli）。当たらなかった・粗い面・空は 0（混ぜるパスで空の色のまま）。
#include "GBuffer.hlsli"
#include "Noise.hlsli"
#include "SsrCommon.hlsli"

Texture2D        gSceneColor : register(t6); // 照らした後のシーンの色（ScreenSpaceReflection.cpp の kSceneColorSlot）
Texture2D<float> gHiZ        : register(t7); // Hi-Z の全部の段（同 kHiZSlot。半分の大きさ。HiZBuffer）

// ScreenSpaceReflection.cpp の SsrConstants と同じ並び。
cbuffer SsrConstants : register(b1)
{
    float4 gSsrInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gSsrProjectionView[4];        // 射影×ビュー（行ごと）。進んだ点を画面へ戻す
    float4 gSsrMarch;                    // x: 計算する粗さの上限  y: 歩数（直線）  z: 一番遠くまで進む長さ（m）  w: 当たったとみなす厚さ（m）
    float4 gSsrFade;                     // x: 画面の端で薄くする幅（画面に対する割合）  y: 全体の強さ  z: ノイズのフレームの番号（TAA）  w: 向きをばらすか（1 / 0）
    float4 gSsrHiZ;                      // x: Hi-Z の段の数  y: Hi-Z で進む回数の上限  z: Hi-Z で進むか（1 / 0）  w: 使わない
};

static const float kRoughnessFadeRange = 0.1; // 粗さの上限の手前この幅で、確からしさを 1 → 0 へ（つやの境目で映り込みがぷつっと切れない）
static const uint kRefineSteps = 4;           // 当たった歩と前の歩の間を詰める二分探索の回数（直線）
static const float kTowardEyeLimit = 0.5;     // 反射と「目への向き」の内積がこれより大きい（60° より目の方を向く）反射は計算しない
static const float4 kNoReflection = float4(0.0, 0.0, 0.0, 0.0);
static const float kNoHit = -1.0;
// Hi-Z：光線の終わりが目の前のこの奥行き（m）より手前へ来ないように長さを切る（目の後ろへ出ると画面へ写せない）。
static const float kHiZMinViewDepth = 0.1;
// Hi-Z：升目の境目を越えるときに、段 0 の 1 画素のこの割合だけ先へ出す（境目の上で止まって、同じ升目を見続けないように）。
static const float kHiZCrossOffset = 0.01;
static const float kHiZFar = 1e30;            // 進む向きの成分が 0 の軸の「境目までの t」（いつまでも越えない）
// ぼけた映り込み。
static const float kStochasticMinRoughness = 0.05; // これよりなめらかな面（水たまり）は鏡の向きのまま（ばらしても見た目が変わらず、ざらつくだけ）
static const float kRoughConfidence = 0.6;          // 粗さ kSsrSharpRoughness + kSsrRoughRange での確からしさ（ばらした 1 本が外れる分を空の色で埋める）
static const float2 kSecondNoiseOffset = float2(37.0, 61.0); // 2 つ目の乱数の IGN を読む画素のずらし（1 つ目と同じ値にならないように）
static const float kTwoPi = 6.28318531;

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

// GGX の見える法線の分布から半ベクトルを 1 本選ぶ（Heitz 2018 の式そのまま）。
// 面の向きが z の空間で、toEye：面から目への向き。alpha：粗さ²（Pbr.hlsli と同じ）。u：0〜1 の乱数 2 つ。
float3 sampleGgxVisibleNormal(float3 toEye, float alpha, float2 u)
{
    float3 stretched = normalize(float3(alpha * toEye.x, alpha * toEye.y, toEye.z));
    float lengthSq = dot(stretched.xy, stretched.xy);
    float3 t1 = (lengthSq > 0.0) ? float3(-stretched.y, stretched.x, 0.0) * rsqrt(lengthSq) : float3(1.0, 0.0, 0.0);
    float3 t2 = cross(stretched, t1);
    float radius = sqrt(u.x);
    float phi = kTwoPi * u.y;
    float p1 = radius * cos(phi);
    float p2 = radius * sin(phi);
    float s = 0.5 * (1.0 + stretched.z);
    p2 = (1.0 - s) * sqrt(1.0 - p1 * p1) + s * p2;
    float3 h = p1 * t1 + p2 * t2 + sqrt(max(0.0, 1.0 - p1 * p1 - p2 * p2)) * stretched;
    return normalize(float3(alpha * h.x, alpha * h.y, max(0.0, h.z)));
}

// 反射の向き。粗い面は GGX でばらした半ベクトルで反射する。ばらした向きが面の中へ入るときは、選び直さずに鏡の向き（ざらつきを増やさない）。
// view：目から画素への向き。n：法線。
float3 reflectionDirection(float3 view, float3 n, float roughness, uint2 halfPixel)
{
    float3 mirror = reflect(view, n);
    if (gSsrFade.w < 0.5 || roughness < kStochasticMinRoughness)
    {
        return mirror;
    }
    // 法線を z にする向きの組（Duff ほか 2017。分かれ道の無い作り方）。
    float side = (n.z >= 0.0) ? 1.0 : -1.0;
    float a = -1.0 / (side + n.z);
    float b = n.x * n.y * a;
    float3 tangent = float3(1.0 + side * n.x * n.x * a, side * b, -side * n.x);
    float3 bitangent = float3(b, side + n.y * n.y * a, -n.y);
    float3 toEye = -view;
    float3 localEye = float3(dot(toEye, tangent), dot(toEye, bitangent), dot(toEye, n));
    float2 u = float2(interleavedGradientNoise(halfPixel, gSsrFade.z), interleavedGradientNoise(halfPixel + kSecondNoiseOffset, gSsrFade.z));
    float3 localHalf = sampleGgxVisibleNormal(localEye, roughness * roughness, u);
    float3 h = localHalf.x * tangent + localHalf.y * bitangent + localHalf.z * n;
    float3 r = reflect(view, h);
    return (dot(n, r) > 0.0) ? r : mirror;
}

float viewDepthOf(float3 world)
{
    return dot(gSsrProjectionView[3], float4(world, 1.0));
}

// 点 world が、その画素に見えている物の奥へ入っているか（入っていて、奥行きの差が厚さより小さい＝当たり）。screen：点を画面へ戻した物。
bool isBehindScene(float3 screen, uint2 depthSize)
{
    float sceneDepth = ssrViewDepthAt(gSceneDepth, gSsrInverseProjectionView, gSsrProjectionView[3], int2(screen.xy * depthSize), depthSize);
    return screen.z > sceneDepth && screen.z - sceneDepth < gSsrMarch.w;
}

bool isOnScreen(float3 screen)
{
    return screen.z > 0.0 && all(screen.xy >= 0.0) && all(screen.xy < 1.0);
}

// 直線で進み、当たった所を二分探索で詰める（今までの進み方）。当たった所までの長さ（m）。当たらなければ kNoHit。
float marchLinear(float3 position, float3 r, uint2 halfPixel, uint2 depthSize)
{
    uint steps = max((uint)gSsrMarch.y, 1u);
    float maxDistance = gSsrMarch.z;
    // 歩き始めを画素ごとにずらす（段々の模様を消し、ぼかしで均す）。TAA のときはフレームごとにも変える（TAA が均す）。
    float jitter = interleavedGradientNoise(halfPixel, gSsrFade.z);
    float previous = 0.0;
    [loop]
    for (uint i = 0; i < steps; ++i)
    {
        // 先ほど歩幅を広げる（近くは細かく、遠くは粗く）。一歩目も少し離して、自分の面に当たらないようにする。
        float ratio = (i + 1.0 + jitter) / (steps + 1.0);
        float travel = maxDistance * ratio * ratio;
        float3 screen = ssrProjectToScreen(gSsrProjectionView, position + r * travel);
        if (!isOnScreen(screen))
        {
            return kNoHit; // 画面の外へ出た（映す物が無い）
        }
        if (isBehindScene(screen, depthSize))
        {
            // 当たった歩と前の歩の間を、二分探索で詰める。
            float lower = previous;
            float upper = travel;
            [unroll]
            for (uint k = 0; k < kRefineSteps; ++k)
            {
                float middle = (lower + upper) * 0.5;
                float3 middleScreen = ssrProjectToScreen(gSsrProjectionView, position + r * middle);
                if (isBehindScene(middleScreen, depthSize))
                {
                    upper = middle;
                }
                else
                {
                    lower = middle;
                }
            }
            return upper;
        }
        previous = travel;
    }
    return kNoHit;
}

// ワールドの位置を、画面の uv と深度の値（0 手前〜1 奥）へ。深度の値は画面の上で直線に変わる（Hi-Z はこの形で進む）。
float3 projectToDevice(float3 world)
{
    float4 p = float4(world, 1.0);
    float4 clip = float4(dot(gSsrProjectionView[0], p), dot(gSsrProjectionView[1], p), dot(gSsrProjectionView[2], p), dot(gSsrProjectionView[3], p));
    float3 ndc = clip.xyz / clip.w;
    return float3(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5, ndc.z);
}

// 段の大きさ（画素）。D3D11 と同じく、段ごとに半分の切り捨て（HiZBuffer の描画先と同じ）。
float2 hiZLevelSize(float2 size0, uint level)
{
    return max(floor(size0 * exp2(-(float)level)), 1.0);
}

// start + direction × t が、升目 cell（cellCount 個に分けた段）の境目を越える t（少し先へ出す）。
float hiZCellExit(float3 start, float3 direction, float2 cell, float2 cellCount, float2 crossOffset)
{
    float2 boundary = (cell + step(0.0, direction.xy)) / cellCount + sign(direction.xy) * crossOffset;
    float2 t = float2(kHiZFar, kHiZFar);
    [unroll]
    for (int axis = 0; axis < 2; ++axis)
    {
        if (direction[axis] != 0.0)
        {
            t[axis] = (boundary[axis] - start[axis]) / direction[axis];
        }
    }
    return min(t.x, t.y);
}

// Hi-Z で進む。当たった所までの長さ（m）。当たらなければ kNoHit。
// 光線は画面の上の (uv, 深度の値) の直線。今の段の升目の一番手前より光線が手前なら、升目の境目まで飛ばして 1 段上へ。
// 升目の中で一番手前に届く・もう奥にいるなら、1 段下へ。段 0 で届いたら当たり。最後に厚さを見る。
float marchHiZ(float3 position, float3 r, uint2 depthSize)
{
    // 光線の長さ：一番遠くまで。目の方へ向かう光線は、目の前 kHiZMinViewDepth に届く所で切る。
    float rayLength = gSsrMarch.z;
    float viewDepthStep = dot(gSsrProjectionView[3].xyz, r); // 1m 進むと奥行きがどれだけ変わるか
    if (viewDepthStep < 0.0)
    {
        rayLength = min(rayLength, (viewDepthOf(position) - kHiZMinViewDepth) / -viewDepthStep);
    }
    if (rayLength <= 0.0)
    {
        return kNoHit;
    }
    float3 start = projectToDevice(position);
    float3 direction = projectToDevice(position + r * rayLength) - start;
    // 画面の外へ出る所で切る（t は 0〜1。1 が光線の終わり）。
    float tEnd = 1.0;
    [unroll]
    for (int axis = 0; axis < 2; ++axis)
    {
        if (direction[axis] > 0.0)
        {
            tEnd = min(tEnd, (1.0 - start[axis]) / direction[axis]);
        }
        else if (direction[axis] < 0.0)
        {
            tEnd = min(tEnd, -start[axis] / direction[axis]);
        }
    }
    uint width, height, levels;
    gHiZ.GetDimensions(0, width, height, levels);
    float2 size0 = float2(width, height);
    uint maxLevel = min((uint)gSsrHiZ.x, levels) - 1;
    float2 crossOffset = kHiZCrossOffset / size0;
    // 自分の升目（段 0）を出た所から始める（自分の面に当たらないように）。
    float t = hiZCellExit(start, direction, floor(start.xy * size0), size0, crossOffset);
    uint level = 0;
    bool hit = false;
    uint iterations = (uint)gSsrHiZ.y;
    [loop]
    for (uint i = 0; i < iterations && t <= tEnd; ++i)
    {
        float3 p = start + direction * t;
        float2 cellCount = hiZLevelSize(size0, level);
        float2 cell = floor(p.xy * cellCount);
        float nearest = gHiZ.Load(int3(cell, level));
        float tCell = hiZCellExit(start, direction, cell, cellCount, crossOffset);
        if (p.z < nearest)
        {
            // 升目の一番手前より手前にいる。奥へ向かう光線が、この升目の中で一番手前の深さに届くか。
            float tSurface = (direction.z > 0.0) ? (nearest - start.z) / direction.z : kHiZFar;
            if (tSurface < tCell)
            {
                t = max(t, tSurface);
                if (level == 0)
                {
                    hit = true;
                    break;
                }
                --level; // 届くかもしれない：細かい段で見直す
            }
            else
            {
                t = tCell; // 升目の中には何も無い：境目の先へ飛ばし、粗い段へ
                level = min(level + 1, maxLevel);
            }
        }
        else
        {
            // もう升目の一番手前より奥にいる（目の方へ向かう光線が手前の物の裏へ入った など）。
            if (level == 0)
            {
                hit = true;
                break;
            }
            --level;
        }
    }
    if (!hit || t > tEnd)
    {
        return kNoHit;
    }
    // 当たった所の厚さ：光線の点が、その画素に見えている物より厚さ以上奥なら、物の裏を通った（当たらなかった）。
    float3 p = start + direction * t;
    float3 world = worldPositionFromDepth(gSsrInverseProjectionView, p.xy, p.z);
    float sceneDepth = ssrViewDepthAt(gSceneDepth, gSsrInverseProjectionView, gSsrProjectionView[3], int2(p.xy * depthSize), depthSize);
    if (viewDepthOf(world) - sceneDepth > gSsrMarch.w)
    {
        return kNoHit;
    }
    return distance(world, position);
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    uint width, height;
    gSceneDepth.GetDimensions(width, height);
    uint2 depthSize = uint2(width, height);
    uint2 halfPixel = (uint2)p.position.xy;
    int3 pixel = int3(halfPixel * kSsrHalfStep, 0);
    float depth = gSceneDepth.Load(pixel);
    float roughness = gGBufferMaterial.Load(pixel).g;
    float maxRoughness = gSsrMarch.x;
    float4 result = kNoReflection;
    if (depth < kFarDepth && roughness < maxRoughness)
    {
        float2 uv = (pixel.xy + 0.5) / float2(depthSize);
        float3 position = worldPositionFromDepth(gSsrInverseProjectionView, uv, depth);
        // 目から画素への向き：同じ画素の一番手前（深度 0）から画素の位置へ（目の位置を別に渡さなくてよい）。
        float3 view = normalize(position - worldPositionFromDepth(gSsrInverseProjectionView, uv, 0.0));
        float3 n = normalize(gGBufferNormal.Load(pixel).xyz);
        float3 r = reflectionDirection(view, n, roughness, halfPixel);
        // ほぼ目の方へ戻る反射（目の後ろへ出ていく）は画面の中で当たらないので、計算しない。
        // 0（目の方へ少しでも戻る反射を全部捨てる）にすると、水たまりを 45° より下向きに見たときの映り込みが全部消える。
        if (dot(r, view) > -kTowardEyeLimit)
        {
            float maxDistance = gSsrMarch.z;
            float hitDistance = (gSsrHiZ.z > 0.5) ? marchHiZ(position, r, depthSize) : marchLinear(position, r, halfPixel, depthSize);
            if (hitDistance > 0.0)
            {
                float3 hit = ssrProjectToScreen(gSsrProjectionView, position + r * hitDistance);
                float3 color = gSceneColor.SampleLevel(gClampSampler, hit.xy, 0).rgb;
                // 確からしさ：画面の端で薄く × つやの境目で薄く × 遠くまで進んだほど薄く × ばらした粗い面で少し薄く。
                float edge = min(min(hit.x, 1.0 - hit.x), min(hit.y, 1.0 - hit.y));
                float roughFade = (gSsrFade.w > 0.5) ? lerp(1.0, kRoughConfidence, ssrRoughBlend(roughness)) : 1.0;
                float confidence = saturate(edge / gSsrFade.x)
                                 * saturate((maxRoughness - roughness) / kRoughnessFadeRange)
                                 * saturate(1.0 - hitDistance / maxDistance)
                                 * roughFade;
                result = float4(color * confidence, confidence) * gSsrFade.y;
            }
        }
    }
    return result;
}
