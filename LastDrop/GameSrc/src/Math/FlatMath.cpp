#include "GameSrc/Math/FlatMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

Vector3 FlatMath::flatten( const Vector3& v ) {
	return Vector3( v.x, 0.f, v.z );
}

float FlatMath::distance( const Vector3& a, const Vector3& b ) {
	float dx = b.x - a.x;
	float dz = b.z - a.z;
	return Math::sqrt( dx * dx + dz * dz );
}
