#include "GameSrc/Math/OrthoBasis.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kWorldRight( 1.f, 0.f, 0.f );
	const float kParallelLimit = 0.99f;	// axis と上の内積がこれを超えたら、ほぼ真上・真下とみなす
}

void OrthoBasis::around( const Vector3& axis, Vector3* right, Vector3* up ) {
	ASSERT( right && up && "OrthoBasis : output is null." );
	const Vector3& helper = ( Math::abs( axis.dot( kWorldUp ) ) > kParallelLimit ) ? kWorldRight : kWorldUp;
	right->setCross( helper, axis );
	right->normalize();
	up->setCross( axis, *right );
}
