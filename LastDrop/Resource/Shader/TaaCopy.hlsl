// TAA の新しい履歴を、シーンの色へ写し戻す（TemporalAntiAliasing::resolve。Taa_Plan.txt 7-1）
// TAA はシーンの色を読みながらシーンの色へは書けないので、いったん新しい履歴へ書き、ここで同じ大きさのシーンの色へそのまま写す
#include "Common.hlsli"

Texture2D gSource : register(t2); // TemporalAntiAliasing.cpp の kCopySourceSlot（新しい履歴）

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex p) : SV_Target
{
    return gSource.Load(int3(p.position.xy, 0));
}
