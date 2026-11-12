// デカール（弾痕など。DecalBatch::draw。SsrDecalMaterial_Plan.txt 8 章）。G-Buffer の色（RT0）と法線（RT1）へ、絵をアルファで混ぜて書く
// 箱（1 辺 1 の立方体）をデカール 1 つごとの行列で描き、画素に見えている面の位置を深度から戻して、箱の中にある所だけに貼る
// （Persson「Volume Decals」）。面そのものに書くので、壁の角・窓枠の端からはみ出さず、面から浮かせる必要も無い
// 箱は裏だけ描き、奥行きの判定はしない（目が箱の中にあっても描けるように。当たっているかはここで決める）
// 描くのは動かない物を G-Buffer へ描いた後・動く物の前（GBuffer::beginDecals〜endDecals）。後から描くキャラがデカールを上書きして消す
#include "Common.hlsli"
#include "DepthPosition.hlsli"

Texture2D        gDecalAlbedo : register(t0); // Manager::setTexture（絵の色とアルファ）
Texture2D        gDecalNormal : register(t1); // Manager::setNormalMap（凹みの法線。BC5 で x・y だけ持つ）
Texture2D<float> gSceneDepth  : register(t5); // GBuffer::beginDecals が付けるシーンの深度（GBuffer.hlsli の t5 と同じ番号）

// DecalBatch.cpp の DecalConstants と同じ並び
cbuffer DecalConstants : register(b1)
{
    float4 gDecalInverseProjectionView[4]; // 射影×ビューの逆行列（行ごと）。深度から位置を戻す
    float4 gDecalToLocal[3];               // ワールドの位置 → 箱の中の座標（-0.5〜0.5）。行ごと
    float4 gDecalRight;                    // xyz: 箱の横の向き（長さ 1。絵の右）  w: 使わない
    float4 gDecalUp;                       // xyz: 箱の縦の向き（長さ 1。絵の上）  w: 使わない
    float4 gDecalFacing;                   // xyz: 貼る面の外向き（長さ 1）  w: 不透明度（古くなると下がる）
    float4 gDecalUvRect;                   // 絵のうち使う所（xy: 左上  zw: 幅・高さ）。何種類かを 1 枚に並べた絵から選ぶ
};

static const float kBoxHalf = 0.5;           // 箱の中の座標の端
static const float kDepthFadeWidth = 0.25;   // 箱の厚さの端（面から離れた所）でこの幅だけ薄くする（箱の中の座標で）
static const float kMinFacing = 0.5;         // 面の向きとデカールの向きの内積がこれより小さい面には貼らない（角の横の面に伸びた跡を付けない）
static const float kFacingFadeRange = 0.2;   // kMinFacing からこの幅で 0 → 1

struct DecalOutput
{
    float4 albedo : SV_Target0;
    float4 normal : SV_Target1;
};

float4 VSMain(StandardVertex v) : SV_Position
{
    return toClip(v.position);
}

DecalOutput PSMain(float4 position : SV_Position)
{
    uint width, height;
    gSceneDepth.GetDimensions(width, height);
    float depth = gSceneDepth.Load(int3(position.xy, 0));
    float2 uv = position.xy / float2(width, height);
    float3 world = worldPositionFromDepth(gDecalInverseProjectionView, uv, depth);
    // 面の向き：隣の画素との位置の差から作り、目の方へ向ける。差分は捨てる（discard）前に取る（2×2 の画素で一緒に計算するため）
    float3 surface = normalize(cross(ddx(world), ddy(world)));
    float3 view = world - worldPositionFromDepth(gDecalInverseProjectionView, uv, 0.0);
    surface = (dot(surface, view) > 0.0) ? -surface : surface;

    float3 local = float3(dot(gDecalToLocal[0], float4(world, 1.0)), dot(gDecalToLocal[1], float4(world, 1.0)), dot(gDecalToLocal[2], float4(world, 1.0)));
    if (depth >= kFarDepth || any(abs(local) > kBoxHalf))
    {
        discard; // 空・箱の外の面には貼らない
    }
    float2 decalUv = float2(local.x + kBoxHalf, kBoxHalf - local.y); // 絵の上が箱の上（gDecalUp）
    decalUv = gDecalUvRect.xy + decalUv * gDecalUvRect.zw;
    float4 color = gDecalAlbedo.Sample(gClampSampler, decalUv);
    float depthFade = saturate((kBoxHalf - abs(local.z)) / kDepthFadeWidth);
    float facingFade = saturate((dot(surface, gDecalFacing.xyz) - kMinFacing) / kFacingFadeRange);
    float alpha = color.a * gDecalFacing.w * depthFade * facingFade;

    // 凹みの法線：絵の向き（横・縦・面の外向き）でワールドへ回す。z は x・y から作り直す（BC5）
    float2 bumpXy = gDecalNormal.Sample(gClampSampler, decalUv).xy * 2.0 - 1.0;
    float bumpZ = sqrt(saturate(1.0 - dot(bumpXy, bumpXy)));
    float3 normal = normalize(gDecalRight.xyz * bumpXy.x + gDecalUp.xyz * bumpXy.y + gDecalFacing.xyz * bumpZ);

    // どちらもアルファで混ぜる（BLEND_LINEAR_KEEP_ALPHA。G-Buffer のアルファは書き換えない）。混ぜた法線は長さが 1 でなくなるので、
    // 読む側（ライティング）で直す
    DecalOutput o;
    o.albedo = float4(color.rgb, alpha);
    o.normal = float4(normal, alpha);
    return o;
}
