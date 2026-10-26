#ifndef INCLUDED_FLAT_MATH_H
#define INCLUDED_FLAT_MATH_H

#include "GameLib/Math/Vector3.h"

// 高さを無視した（真上から見た）計算の、ただ1か所。状態は持たない。
// 移動の当たり判定・ゾンビが追う距離・道の図が使う（どれも「床の上でどれだけ離れているか」を知りたいだけで、段差の高さは数えない）。
class FlatMath{
public:
	// v の y を 0 にした物。
	static GameLib::Math::Vector3 flatten( const GameLib::Math::Vector3& v );
	// a と b の水平の距離（m）。
	static float distance( const GameLib::Math::Vector3& a, const GameLib::Math::Vector3& b );
};

#endif
