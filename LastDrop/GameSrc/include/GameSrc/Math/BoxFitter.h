#ifndef INCLUDED_BOX_FITTER_H
#define INCLUDED_BOX_FITTER_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include <vector>

// 点の集まり（部位の頂点）に沿った向きの箱を求める計算だけを持つ。状態は持たない
// 箱の向きは次の順で決める
//   1. 長い辺：渡された向き（関節を結ぶ向き）。渡されなければ、点の広がりが一番大きい向き（主成分分析）
//   2. 残り2辺：長い辺に直交する面へ点を落とし、その面の中で広がりが一番大きい向きと、それに直交する向き
// 向きを決めた後、その向きで点を囲む（向きが点に沿うので、斜めに伸びた腕でも細長い箱になる）
class BoxFitter{
public:
	// points を囲む箱を、-1〜1 の立方体を置く行列として boxOut に書く（HitboxSet の箱と同じ表し方）
	// longAxis: 長い辺の向き（長さは問わない）。nullptr、または長さがほぼ 0 なら点の広がりから決める
	// minHalfExtent: 幅の半分の下限（点が平らに並んで幅が 0 になると、逆行列が作れないため）
	// boxOut: 出力先。nullptr 不可。points が空なら false（出力は変えない）
	static bool fit( const std::vector< GameLib::Math::Vector3 >& points, const GameLib::Math::Vector3* longAxis,
		float minHalfExtent, GameLib::Math::Matrix34* boxOut );
};

#endif
