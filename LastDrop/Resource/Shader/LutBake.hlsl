// 今の色の調整（Grading.hlsli の gradeColor）を、色の表（LUT。1024×32 の帯）に焼く（PostProcess の F6 の Export LUT。MoonStarsLut_Plan.txt 6-3）
// 描画先の 1 画素が表の 1 段。恒等の表の色（画面の色）をリニアに直して gradeColor に通し、画面の色に戻して書く
// 定数は Present.hlsl と同じ PresentConstants（b1）。焼くときはグレーディングを切っていても、今の値を入れて付ける（PostProcess::exportLut）
#include "Common.hlsli"
#include "Grading.hlsli"

FullScreenVertex VSMain(uint id : SV_VertexID)
{
    return fullScreenVertex(id);
}

float4 PSMain(FullScreenVertex v) : SV_Target
{
    float3 identity = lutIdentityColor(uint2(v.position.xy));
    return float4(linearToSrgb(gradeColor(srgbToLinear(identity))), 1.0);
}
