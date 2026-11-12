#include "GameSrc/Math/AngleMath.h"

#include "GameLib/Math/Functions.h"
#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalfTurn = 180.f;
	const float kFullTurn = 360.f;
}

float AngleMath::wrapSigned( float degree ) {
	// -180 を 0 周目の始まりにして、何周目かを floor で数えて引く（while で 1 周ずつ引くと、無限大で終わらない）
	float wrapped = degree - kFullTurn * std::floor( ( degree + kHalfTurn ) / kFullTurn );
	// -180 のすぐ下（-180.00001 など）は、360 を足した所で丸められてちょうど 180 になることがある。範囲の外なので戻す
	return ( wrapped >= kHalfTurn ) ? wrapped - kFullTurn : wrapped;
}

float AngleMath::difference( float target, float current ) {
	return wrapSigned( target - current );
}

float AngleMath::headingOf( const Vector3& direction ) {
	if ( direction.x == 0.f && direction.z == 0.f ) {
		return 0.f;
	}
	return Math::atan2( direction.x, direction.z );
}
