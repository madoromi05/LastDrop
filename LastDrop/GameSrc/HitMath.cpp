#include "HitMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kCubeHalf = 1.f;           // 立方体の幅の半分（-1〜1）
	const float kMinMove = 0.000001f;      // これより小さい移動は、その軸には動かないとみなす（0 割りを避ける）
	const float kStartTime = 0.f;
	const float kEndTime = 1.f;
	const int kAxisNumber = 3;
}

bool HitMath::segmentUnitCube( const Vector3& origin, const Vector3& delta, float* time ) {
	// 軸ごとに「入る時刻・出る時刻」を求め、一番遅く入る時刻が一番早く出る時刻より前なら当たり（スラブ法）。
	float enterTime = kStartTime; // 線分の始点より前は見ない（始点が中なら 0 のまま）
	float exitTime = kEndTime;
	for ( int axis = 0; axis < kAxisNumber; ++axis ) {
		float start = origin[ axis ];
		float move = delta[ axis ];
		if ( Math::abs( move ) < kMinMove ) {
			if ( Math::abs( start ) > kCubeHalf ) {
				return false; // この軸には動かず、幅の外にいる
			}
			continue;
		}
		float toLow = ( -kCubeHalf - start ) / move;
		float toHigh = ( kCubeHalf - start ) / move;
		enterTime = Math::max( enterTime, Math::min( toLow, toHigh ) );
		exitTime = Math::min( exitTime, Math::max( toLow, toHigh ) );
		if ( enterTime > exitTime ) {
			return false;
		}
	}
	*time = enterTime;
	return true;
}
