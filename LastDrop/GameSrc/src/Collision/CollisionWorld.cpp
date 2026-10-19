#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/IGroundHeight.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

#include <algorithm>
#include <utility>

using namespace GameLib;
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

	// 真上から見た、軸に沿った範囲（x・z の最小〜最大）。遠い面のかけらを、細かい判定の前に捨てるために使う。
	struct Reach{
		Vector3 low;
		Vector3 high;
		// low〜high の範囲と重なる（接するも含む）か。y は見ない。
		bool touches( const Vector3& otherLow, const Vector3& otherHigh ) const {
			return low.x <= otherHigh.x && otherLow.x <= high.x && low.z <= otherHigh.z && otherLow.z <= high.z;
		}
	};

	// box を delta だけ動かす間に届く範囲。長方形は、向きによらず囲める円（中心から隅までの長さ）で見積もる。
	Reach reachOf( const BoxCollider& box, const Vector3& delta ) {
		float radius = Math::sqrt( box.halfWidthX * box.halfWidthX + box.halfWidthZ * box.halfWidthZ );
		Vector3 end;
		end.setAdd( box.center, delta );
		Reach reach;
		reach.low = box.center;
		reach.low.min( end );
		reach.low -= Vector3( radius, 0.f, radius );
		reach.high = box.center;
		reach.high.max( end );
		reach.high += Vector3( radius, 0.f, radius );
		return reach;
	}
}

CollisionWorld::CollisionWorld() :
	mGround( nullptr ) {
}

void CollisionWorld::addBox( const BoxCollider& box ) {
	mBoxes.push_back( box );
}

void CollisionWorld::addWallMesh( WallMesh walls ) {
	mWallMeshes.push_back( std::move( walls ) );
}

void CollisionWorld::addBody( const ICollisionBody* body ) {
	ASSERT( body && "CollisionWorld : body is null." );
	mBodies.push_back( body );
}

void CollisionWorld::removeBody( const ICollisionBody* body ) {
	mBodies.erase( std::remove( mBodies.begin(), mBodies.end(), body ), mBodies.end() );
}

void CollisionWorld::setGround( const IGroundHeight* ground ) {
	mGround = ground;
}

bool CollisionWorld::groundHeight( const Vector3& position, float* height ) const {
	if ( !mGround ) {
		return false;
	}
	return mGround->heightBelow( position, height );
}

bool CollisionWorld::sweep( const BoxCollider& moving, const Vector3& delta, const ICollisionBody* ignore, SweepHit* hit ) const {
	SweepHit candidate;
	bool found = false;
	for ( const BoxCollider& wall : mBoxes ) {
		if ( SweepMath::sweepBox( moving, delta, wall, &candidate ) ) {
			keepEarlier( candidate, hit, &found );
		}
	}
	Reach reach = reachOf( moving, delta );
	for ( const WallMesh& walls : mWallMeshes ) {
		if ( !reach.touches( walls.boundsMin(), walls.boundsMax() ) ) {
			continue;
		}
		for ( const WallPiece& piece : walls.pieces() ) {
			if ( !reach.touches( piece.boundsMin, piece.boundsMax ) ) {
				continue;
			}
			if ( SweepMath::sweepPiece( moving, delta, piece, &candidate ) ) {
				keepEarlier( candidate, hit, &found );
			}
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

const std::vector< WallMesh >& CollisionWorld::wallMeshes() const {
	return mWallMeshes;
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
	Reach reach = reachOf( moving, Vector3( 0.f, 0.f, 0.f ) );
	for ( const WallMesh& walls : mWallMeshes ) {
		if ( !reach.touches( walls.boundsMin(), walls.boundsMax() ) ) {
			continue;
		}
		for ( const WallPiece& piece : walls.pieces() ) {
			if ( !reach.touches( piece.boundsMin, piece.boundsMax ) ) {
				continue;
			}
			if ( SweepMath::penetratePiece( moving, piece, &candidate ) ) {
				keepDeeper( candidate, push, &found );
			}
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
