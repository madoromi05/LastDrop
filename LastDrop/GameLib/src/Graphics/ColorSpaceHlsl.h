#ifndef INCLUDED_GAMELIB_GRAPHICS_COLORSPACE_HLSL_H
#define INCLUDED_GAMELIB_GRAPHICS_COLORSPACE_HLSL_H

// 固定パイプラインの頂点シェーダで使う、頂点色をリニアへ直す関数（HLSL の文字列）。
// シェーダの文字列の前に並べて書くと、1 つの文字列につながる（"..." R"HLSL(...)HLSL" は隣り合う文字列の連結）。
// 式は ColorSpace.h（CPU 側）と Resource/Shader/Common.hlsli（自作シェーダ）の srgbToLinear と同じ。
// 直すかどうかは b0 の gColorParams.x（Manager::enableLinearColor）。中間の描画先へ 3D を描く間だけ 1 にする。
#define GAMELIB_COLOR_SPACE_HLSL \
	"float3 srgbToLinear(float3 c)\n" \
	"{\n" \
	"    return (c <= 0.04045) ? (c / 12.92) : pow(abs((c + 0.055) / 1.055), 2.4);\n" \
	"}\n" \
	"float4 vertexColorToScene(float4 color, float4 colorParams)\n" \
	"{\n" \
	"    float4 c = color.zyxw;\n" \
	"    if (colorParams.x > 0.5) c.rgb = srgbToLinear(c.rgb);\n" \
	"    return c;\n" \
	"}\n"

#endif
