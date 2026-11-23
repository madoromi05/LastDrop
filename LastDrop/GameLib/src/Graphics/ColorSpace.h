#ifndef INCLUDED_GAMELIB_GRAPHICS_COLORSPACE_H
#define INCLUDED_GAMELIB_GRAPHICS_COLORSPACE_H

#include "GameLib/Math/Vector3.h"
#include <cmath>

// 画像の色（sRGB。ガンマが掛かった数）と、光の量に比例した数（リニア）の変換（CPU 側）
// 3D の絵をリニアで計算するために使う
// pow 2.2 の近似ではなく sRGB の正しい式にする（GPU が _SRGB のテクスチャを読むときの式と同じにするため）
// シェーダ側の同じ式は ColorSpaceHlsl.h（固定パイプライン）と Resource/Shader/Common.hlsli（自作シェーダ）
namespace GameLib{
namespace Graphics{
namespace ColorSpace{

// 0〜1 の画像の色 1 つを、リニアへ直す
inline float srgbToLinear( float c ){
	return ( c <= 0.04045f ) ? ( c / 12.92f ) : std::pow( ( c + 0.055f ) / 1.055f, 2.4f );
}
// 0〜1 のリニアの数 1 つを、画像の色へ戻す
inline float linearToSrgb( float c ){
	return ( c <= 0.0031308f ) ? ( c * 12.92f ) : ( 1.055f * std::pow( c, 1.f / 2.4f ) - 0.055f );
}

// C++ で選んだ色（材質・ライト・環境光）をリニアへ直す
// 1 を超える色（LitQuadBatch のマズルフラッシュの芯の 3 倍など）は「1 以下の色 × 明るさの倍率」とみなし、
// 色の部分だけを直して倍率はそのまま掛ける（全体を式に通すと 3 が 14 倍になり、選んだ明るさの意味が変わるため）
inline Math::Vector3 srgbToLinear( const Math::Vector3& color ){
	float scale = color.x;
	if ( color.y > scale ) scale = color.y;
	if ( color.z > scale ) scale = color.z;
	if ( scale <= 1.f ){
		return Math::Vector3( srgbToLinear( color.x ), srgbToLinear( color.y ), srgbToLinear( color.z ) );
	}
	return Math::Vector3(
		srgbToLinear( color.x / scale ) * scale,
		srgbToLinear( color.y / scale ) * scale,
		srgbToLinear( color.z / scale ) * scale );
}

} //namespace ColorSpace
} //namespace Graphics
} //namespace GameLib

#endif
