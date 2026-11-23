// Hi-Z（深度の縮小の段）を 1 段作る（HiZBuffer::draw）。段 0 はシーンの深度（元の大きさ）から、
// 段 i は段 i−1 から作る。描く先の 1 画素に重なる元の画素の深度のうち、一番手前（最小。0 が手前・1 が奥）を書く
// 元の大きさが描く先のちょうど 2 倍でないとき（奇数の段・画面の大きさが奇数）は、重なる元の画素を 3 つ目まで見る
// （取りこぼすと、細い物の手前を見落として、光線がその物を抜ける）
#include "Common.hlsli"

Texture2D<float> gHiZSource : register(t2); // HiZBuffer.cpp の kSourceSlot（段 0 はシーンの深度、それより後は 1 つ前の段）

// HiZBuffer.cpp の HiZBuildConstants と同じ並び
cbuffer HiZBuildConstants : register(b1)
{
    float4 gHiZTargetSize; // xy: 描く先の段の大きさ（画素）  zw: 使わない
};

static const uint kMaxFootprint = 3; // 描く先の 1 画素に重なる元の画素の数（一辺）の上限。比が 2 倍より少し大きいときの 3

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float PSMain(FullScreenVertex p) : SV_Target
{
    uint sourceWidth, sourceHeight;
    gHiZSource.GetDimensions(sourceWidth, sourceHeight);
    uint2 sourceSize = uint2(sourceWidth, sourceHeight);
    uint2 targetSize = (uint2)gHiZTargetSize.xy;
    uint2 pixel = (uint2)p.position.xy;
    // 描く先の画素 [pixel, pixel+1) に重なる元の画素の範囲（始まりは切り捨て・終わりは切り上げ。少なくとも 1 つ）
    uint2 begin = pixel * sourceSize / targetSize;
    uint2 end = min(((pixel + 1) * sourceSize + targetSize - 1) / targetSize, sourceSize);
    end = max(end, begin + 1);
    float nearest = 1.0;
    [unroll]
    for (uint y = 0; y < kMaxFootprint; ++y)
    {
        [unroll]
        for (uint x = 0; x < kMaxFootprint; ++x)
        {
            uint2 source = begin + uint2(x, y);
            if (all(source < end))
            {
                nearest = min(nearest, gHiZSource.Load(int3(source, 0)));
            }
        }
    }
    return nearest;
}
