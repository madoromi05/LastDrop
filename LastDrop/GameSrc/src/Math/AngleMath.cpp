#include "GameSrc/Math/AngleMath.h"

#include "GameLib/Math/Functions.h"
#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

float AngleMath::wrapSigned( float degree ) {
	float wrapped = degree - kFullTurnDegree * std::floor( ( degree + kHalfTurnDegree ) / kFullTurnDegree );	// -180 を 0 周目の始まりにして、何周目かを floor で数えて引く（while で 1 周ずつ引くと、無限大で終わらない）
	return ( wrapped >= kHalfTurnDegree ) ? wrapped - kFullTurnDegree : wrapped;								// -180 のすぐ下（-180.00001 など）は、360 を足した所で丸められてちょうど 180 になることがある。範囲の外なので戻す
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

Vector3 AngleMath::forwardOf( float degree ) {
	return Vector3( Math::sin( degree ), 0.f, Math::cos( degree ) );
}

Vector3 AngleMath::rightOf( float degree ) {
	return Vector3( -Math::cos( degree ), 0.f, Math::sin( degree ) );
}

Vector3 AngleMath::directionOf( float yawDegree, float pitchDegree ) {
	return Vector3(
		Math::sin( yawDegree ) * Math::cos( pitchDegree ),
		-Math::sin( pitchDegree ),
		Math::cos( yawDegree ) * Math::cos( pitchDegree ) );
}
