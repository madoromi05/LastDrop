#ifndef INCLUDED_GAMELIB_GRAPHICS_ENUM_H
#define INCLUDED_GAMELIB_GRAPHICS_ENUM_H

namespace GameLib{
namespace Graphics{

//グローバル列挙群
enum BlendMode{
	BLEND_OPAQUE,
	BLEND_LINEAR,
	BLEND_ADDITIVE,

	BLEND_MAX,
};

enum CullMode{
	CULL_NONE,
	CULL_FRONT,
	CULL_BACK,

	CULL_MAX,
};

enum PrimitiveType{
	PRIMITIVE_TRIANGLE,
	PRIMITIVE_LINE,
	PRIMITIVE_POINT,
};

enum TextureFilter{
	TEXTURE_FILTER_POINT,
	TEXTURE_FILTER_LINEAR,
};

enum LightingMode{
	LIGHTING_NONE,
	LIGHTING_PER_VERTEX,
	LIGHTING_PER_PIXEL,
};

///描画先（RenderTarget）の画素の形式
enum RenderTargetFormat{
	RENDER_TARGET_RGBA8,   ///8bit×4。色（0〜1）
	RENDER_TARGET_RGBA16F, ///16bit浮動小数×4。1を超える明るさを持てる色（トーンマップ前の画面など）
	RENDER_TARGET_R32F,    ///32bit浮動小数×1。深度・距離など1つの値
	RENDER_TARGET_RG16F,   ///16bit浮動小数×2。分散シャドウマップ（深度と深度の2乗）など2つの値
};

///頂点の1要素の種類（VertexFormat で並べる）。HLSL 側のセマンティクスは各行のとおり
enum VertexElement{
	VERTEX_POSITION4,     ///float4 POSITION
	VERTEX_NORMAL3,       ///float3 NORMAL
	VERTEX_COLOR,         ///float4 COLOR0（8bit×4 BGRA を 0〜1 で読む）
	VERTEX_TEXCOORD2,     ///float2 TEXCOORD0
	VERTEX_TANGENT4,      ///float4 TANGENT（w は従法線の向き）
	VERTEX_BONE_INDICES4, ///uint4 BLENDINDICES（8bit×4）
	VERTEX_BONE_WEIGHTS4, ///float4 BLENDWEIGHT
};

} //namespace Graphics
} //namespace GameLib

#endif
