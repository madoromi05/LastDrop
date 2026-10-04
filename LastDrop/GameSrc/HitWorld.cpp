#include "HitWorld.h"
#include "IHittable.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

void HitWorld::addTarget( const IHittable* target ) {
	ASSERT( target && "HitWorld : target is null." );
	mTargets.push_back( target );
}

bool HitWorld::raycast( const Vector3& origin, const Vector3& delta, const IHittable* ignore,
const IHittable** targetOut, HitResult* hit ) const {
	bool found = false;
	for ( const IHittable* target : mTargets ) {
		if ( target == ignore ) {
			continue;
		}
		HitResult candidate;
		if ( !target->hitboxes().raycast( origin, delta, &candidate ) ) {
			continue;
		}
		if ( found && candidate.time >= hit->time ) {
			continue;
		}
		*hit = candidate;
		if ( targetOut ) {
			*targetOut = target;
		}
		found = true;
	}
	return found;
}

const std::vector< const IHittable* >& HitWorld::targets() const {
	return mTargets;
}
