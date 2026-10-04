#include "CollisionWorld.h"
#include "ICollisionBody.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// candidate が best より早ければ best を置き換える。
	void keepEarlier( const SweepHit& candidate, SweepHit* best, bool* found ) {
		if ( !*found || candidate.time < best->time ) {
			*best = candidate;
			*found = true;
		}
	}

	// candidate が best より深ければ best を置き換える。
	void keepDeeper( const Vector3& candidate, Vector3* best, bool* found ) {
		if ( !*found || candidate.squareLength() > best->squareLength() ) {
			*best = candidate;
			*found = true;
		}
	}
}

void CollisionWorld::addBox( const BoxCollider& box ) {
	mBoxes.push_back( box );
}

void CollisionWorld::addBody( const ICollisionBody* body ) {
	ASSERT( body && "CollisionWorld : body is null." );
	mBodies.push_back( body );
}

bool CollisionWorld::sweep( const BoxCollider& moving, const Vector3& delta, const ICollisionBody* ignore, SweepHit* hit ) const {
	SweepHit candidate;
	bool found = false;
	for ( const BoxCollider& wall : mBoxes ) {
		if ( SweepMath::sweepBox( moving, delta, wall, &candidate ) ) {
			keepEarlier( candidate, hit, &found );
		}
	}
	for ( const ICollisionBody* body : mBodies ) {
		if ( body == ignore ) {
			continue;
		}
		if ( SweepMath::sweepBox( moving, delta, body->collider(), &candidate ) ) {
			keepEarlier( candidate, hit, &found );
		}
	}
	return found;
}

const std::vector< BoxCollider >& CollisionWorld::boxes() const {
	return mBoxes;
}

const std::vector< const ICollisionBody* >& CollisionWorld::bodies() const {
	return mBodies;
}

bool CollisionWorld::findPenetration( const BoxCollider& moving, const ICollisionBody* ignore, Vector3* push ) const {
	Vector3 candidate;
	bool found = false;
	for ( const BoxCollider& wall : mBoxes ) {
		if ( SweepMath::penetrateBox( moving, wall, &candidate ) ) {
			keepDeeper( candidate, push, &found );
		}
	}
	for ( const ICollisionBody* body : mBodies ) {
		if ( body == ignore ) {
			continue;
		}
		if ( SweepMath::penetrateBox( moving, body->collider(), &candidate ) ) {
			keepDeeper( candidate, push, &found );
		}
	}
	return found;
}
