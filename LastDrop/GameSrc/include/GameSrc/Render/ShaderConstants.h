#ifndef INCLUDED_SHADER_CONSTANTS_H
#define INCLUDED_SHADER_CONSTANTS_H

namespace GameLib{
	namespace Math{
		class Matrix44;
		class Matrix34;
	}
}

// 行列を定数バッファ（HLSL の cbuffer）の float の並びへ詰める、ただ1か所。状態は持たない
// HLSL 側は row_major で読むので、行ごと（m00, m01, …）の順に並べる。並べ方を変えるときは、ここと HLSL を直す
class ShaderConstants{
public:
	static const int kMatrixFloats = 16;	// 4×4 の行列の float の数
	static const int kRowFloats = 4;		// 行列の 1 行・float4 1 つの float の数
	static const int kMatrix34Rows = 3;		// 3×4 の行列の行の数（float4 が 3 つ）
	static const int kMatrix34Floats = kMatrix34Rows * kRowFloats;

	static void copyMatrix44( const GameLib::Math::Matrix44& matrix, float* destination );			// m00..m33 を destination へ 16 個書く
	// 射影×ビューの 4 行目（m30..m33）を destination へ 4 個書く。ワールドの位置との内積が、目からの奥行きになる行
	static void copyDepthRow( const GameLib::Math::Matrix44& projectionView, float* destination );
	static void copyMatrix34( const GameLib::Math::Matrix34& matrix, float* destination );			// m00..m23 を destination へ 12 個書く（float4 の 3 行）
};

#endif
