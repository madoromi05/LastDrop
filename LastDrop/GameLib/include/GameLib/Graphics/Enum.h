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

///テクスチャの画素が何を表すか（Texture::create で指定する）。
///色の画像を SRGB にすると、GPU が読むときにリニア（光の量に比例した数）へ直してくれる。
enum TextureColorSpace{
	TEXTURE_COLOR_SPACE_LINEAR, ///読んだ数をそのまま使う。色ではない数（法線マップ）と、そのまま画面へ出す画像（HUD・文字）
	TEXTURE_COLOR_SPACE_SRGB,   ///色の画像（アルベド・エフェクト）。ミップもリニアで平均して作る
};

///描画先（RenderTarget）の画素の形式
enum RenderTargetFormat{
	RENDER_TARGET_RGBA8,   ///8bit×4。色（0〜1）
	RENDER_TARGET_RGBA16F, ///16bit浮動小数×4。1を超える明るさを持てる色（トーンマップ前の画面など）
	RENDER_TARGET_R32F,    ///32bit浮動小数×1。深度・距離など1つの値
	RENDER_TARGET_RG16F,   ///16bit浮動小数×2。分散シャドウマップ（深度と深度の2乗）など2つの値
	///8bit×4 を sRGB で持つ。シェーダはリニアの数で書き、読むとリニアへ戻る（GPU が直す）。
	///暗い色を細かく持てるので、8bit に入れる色（G-Buffer のアルベドなど）に使う
	RENDER_TARGET_RGBA8_SRGB,
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
