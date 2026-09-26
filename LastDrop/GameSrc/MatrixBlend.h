#ifndef INCLUDED_MATRIX_BLEND_H
#define INCLUDED_MATRIX_BLEND_H

#include "GameLib/Math/Matrix34.h"

// 姿勢行列（移動・回転・拡大だけを含み、せん断の無い行列）を2つ混ぜる。
// アニメの切り替えで、前のポーズと次のポーズをボーンごとに少しずつ混ぜる（クロスフェード）ために使う。
//
// 行列の成分をそのまま線形補間すると、回転の途中で軸が縮んで「細くなる・潰れる」崩れが出る
// （例: 腕を大きく振る途中で腕が短くなる）。そこで 移動=線形補間、回転=四元数の球面補間(slerp)、
// 拡大=軸ごとの線形補間 に分けて混ぜ、組み立て直す。
class MatrixBlend{
public:
	// from と to を t の割合で混ぜて out へ書く。t=0 で from、t=1 で to。
	// out は from / to と同じものを指してもよい。nullptr 不可。
	static void blend(
		const GameLib::Math::Matrix34& from,
		const GameLib::Math::Matrix34& to,
		float t,
		GameLib::Math::Matrix34* out );
};

#endif
