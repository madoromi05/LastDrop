#ifndef INCLUDED_HIT_MATH_H
#define INCLUDED_HIT_MATH_H

#include "GameLib/Math/Vector3.h"

// 命中判定（弾の通り道の線分と、部位の箱）の計算だけを持つ。状態は持たない。
// 箱は「-1〜1 の立方体を行列で置いたもの」として扱う。線分を箱の空間へ移せば、どんな向き・大きさの箱も
// この立方体との判定で済む（行列は移動・回転・拡大だけなので、線分上の時刻は空間を移しても変わらない）。
class HitMath{
public:
	// origin から delta だけ進む線分が、-1〜1 の立方体に最初に入る時刻（0〜1）を time に書いて true。
	// 始点が立方体の中なら time=0 で当たり。time: 出力先。nullptr 不可。
	static bool segmentUnitCube( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, float* time );
};

#endif
