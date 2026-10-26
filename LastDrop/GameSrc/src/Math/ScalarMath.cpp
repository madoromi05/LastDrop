#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/Math/Functions.h"
#include <cmath>

using namespace GameLib;

namespace{
	const float kFullCycle = 1.f;
}

float ScalarMath::lerp( float a, float b, float t ) {
	return a + ( b - a ) * t;
}

float ScalarMath::clamp( float value, float low, float high ) {
	return Math::max( low, Math::min( high, value ) );
}

float ScalarMath::clampSigned( float value, float limit ) {
	return clamp( value, -limit, limit );
}

float ScalarMath::smoothStep( float t ) {
	return t * t * ( 3.f - 2.f * t );
}

float ScalarMath::wrapUnit( float phase ) {
	float wrapped = phase - std::floor( phase );
	// 0 のすぐ下（-0.00000001 など）は、引いた所で丸められてちょうど 1 になることがある。範囲の外なので 0 にする。
	return ( wrapped >= kFullCycle ) ? 0.f : wrapped;
}
