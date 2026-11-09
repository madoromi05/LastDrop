#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/IGroundHeight.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

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

	// ますの大きさ（m）。体（幅 0.5m 前後）の数倍にして、1 回の問い合わせで引くますを 1〜4 個にする。
	const float kGridCellSize = 4.f;
	// 動く体をますへ入れる時に足す余裕（m）。ますを入れ直すのはティックの始めだけなので、そのティックの中で動ける分
	// （走っても 1 ティック 0.1m 前後）より十分大きくする。
	const float kBodyGridMargin = 0.5f;

	// 長方形を向きによらず囲める円の半径（中心から隅までの長さ）。
	float coverRadius( const BoxCollider& box ) {
		return Math::sqrt( box.halfWidthX * box.halfWidthX + box.halfWidthZ * box.halfWidthZ );
	}

	// 長方形を真上から囲む範囲に margin を足した物。
	Reach boundsOf( const BoxCollider& box, float margin ) {
		float radius = coverRadius( box ) + margin;
		Reach reach;
		reach.low.setSub( box.center, Vector3( radius, 0.f, radius ) );
		reach.high.setAdd( box.center, Vector3( radius, 0.f, radius ) );
		return reach;
	}

	// box を delta だけ動かす間に届く範囲。長方形は、向きによらず囲める円（中心から隅までの長さ）で見積もる。
	Reach reachOf( const BoxCollider& box, const Vector3& delta ) {
		float radius = coverRadius( box );
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
	mGround( nullptr ),
	mStaticGridDirty( true ),
	mBodyGridDirty( true ) {
}

void CollisionWorld::addBox( const BoxCollider& box ) {
	mBoxes.push_back( box );
	mStaticGridDirty = true;
}

void CollisionWorld::addWallMesh( WallMesh walls ) {
	mWallMeshes.push_back( std::move( walls ) );
	mStaticGridDirty = true;
}

void CollisionWorld::addBody( const ICollisionBody* body ) {
	ASSERT( body && "CollisionWorld : body is null." );
	mBodies.push_back( body );
	mBodyGridDirty = true; // ティックの途中で湧いた体も、次の問い合わせから見える
}

void CollisionWorld::removeBody( const ICollisionBody* body ) {
	mBodies.erase( std::remove( mBodies.begin(), mBodies.end(), body ), mBodies.end() );
	mBodyGridDirty = true; // 外すと後ろの体の番号がずれるので、入れ直す
}

void CollisionWorld::setGround( const IGroundHeight* ground ) {
	mGround = ground;
}

void CollisionWorld::refreshBodies() {
	mBodyGridDirty = true;
}

void CollisionWorld::updateGrids() const {
	if ( mStaticGridDirty ) {
		// ますで覆う範囲は、動かない物を全部囲む箱に、ます 1 つ分の余裕を足した物。範囲の外の体は端のますに入るので、外へ出ても漏れない。
		Reach area{ Vector3( FLOAT_MAX ), Vector3( -FLOAT_MAX ) };
		for ( const BoxCollider& box : mBoxes ) {
			Reach reach = boundsOf( box, 0.f );
			area.low.min( reach.low );
			area.high.max( reach.high );
		}
		for ( const WallMesh& walls : mWallMeshes ) {
			area.low.min( walls.boundsMin() );
			area.high.max( walls.boundsMax() );
		}
		if ( area.low.x > area.high.x ) { // 動かない物が無い（まだ町を作っていない）
			area.low.set( 0.f, 0.f, 0.f );
			area.high.set( 0.f, 0.f, 0.f );
		}
		area.low -= Vector3( kGridCellSize, 0.f, kGridCellSize );
		area.high += Vector3( kGridCellSize, 0.f, kGridCellSize );
		mBoxGrid.reset( area.low, area.high, kGridCellSize );
		mWallGrid.reset( area.low, area.high, kGridCellSize );
		mBodyGrid.reset( area.low, area.high, kGridCellSize );
		for ( size_t i = 0; i < mBoxes.size(); ++i ) {
			Reach reach = boundsOf( mBoxes[ i ], 0.f );
			mBoxGrid.add( static_cast< int >( i ), reach.low, reach.high );
		}
		for ( size_t i = 0; i < mWallMeshes.size(); ++i ) {
			mWallGrid.add( static_cast< int >( i ), mWallMeshes[ i ].boundsMin(), mWallMeshes[ i ].boundsMax() );
		}
		mStaticGridDirty = false;
		mBodyGridDirty = true; // reset で体のますも空になった
	}
	if ( mBodyGridDirty ) {
		fillBodyGrid();
		mBodyGridDirty = false;
	}
}

void CollisionWorld::fillBodyGrid() const {
	mBodyGrid.clearItems();
	for ( size_t i = 0; i < mBodies.size(); ++i ) {
		Reach reach = boundsOf( mBodies[ i ]->collider(), kBodyGridMargin );
		mBodyGrid.add( static_cast< int >( i ), reach.low, reach.high );
	}
}

bool CollisionWorld::groundHeight( const Vector3& position, float* height ) const {
	if ( !mGround ) {
		return false;
	}
	return mGround->heightBelow( position, height );
}

bool CollisionWorld::BodyFilter::skips( const ICollisionBody* body ) const {
	return skipAllBodies || body == self || ( passThrough && body == passThrough );
}

bool CollisionWorld::sweep( const BoxCollider& moving, const Vector3& delta, const ICollisionBody* ignore, SweepHit* hit ) const {
	return sweep( moving, delta, BodyFilter{ ignore, nullptr, false }, hit );
}

bool CollisionWorld::findPenetration( const BoxCollider& moving, const ICollisionBody* ignore, Vector3* push ) const {
	return findPenetration( moving, BodyFilter{ ignore, nullptr, false }, push );
}

bool CollisionWorld::sweep( const BoxCollider& moving, const Vector3& delta, const BodyFilter& filter, SweepHit* hit ) const {
	updateGrids();
	SweepHit candidate;
	bool found = false;
	Reach reach = reachOf( moving, delta );
	mBoxGrid.collectArea( reach.low, reach.high, &mCandidates );
	for ( int index : mCandidates ) {
		if ( SweepMath::sweepBox( moving, delta, mBoxes[ index ], &candidate ) ) {
			keepEarlier( candidate, hit, &found );
		}
	}
	mWallGrid.collectArea( reach.low, reach.high, &mCandidates );
	for ( int index : mCandidates ) {
		const WallMesh& walls = mWallMeshes[ index ];
		if ( !reach.touches( walls.boundsMin(), walls.boundsMax() ) ) {
			continue;
		}
		for ( const WallPiece& piece : walls.pieces() ) {
			if ( !reach.touches( piece.boundsMin, piece.boundsMax ) ) {
				continue;
			}
			PROFILE_COUNT( "Collide.walls", 1 ); // 細かく比べた壁のかけら
			if ( SweepMath::sweepPiece( moving, delta, piece, &candidate ) ) {
				keepEarlier( candidate, hit, &found );
			}
		}
	}
	if ( !filter.skipAllBodies ) {
		mBodyGrid.collectArea( reach.low, reach.high, &mCandidates );
		for ( int index : mCandidates ) {
			const ICollisionBody* body = mBodies[ index ];
			if ( filter.skips( body ) ) {
				continue;
			}
			PROFILE_COUNT( "Collide.bodies", 1 ); // 細かく比べた動く体
			if ( SweepMath::sweepBox( moving, delta, body->collider(), &candidate ) ) {
				keepEarlier( candidate, hit, &found );
			}
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

bool CollisionWorld::findPenetration( const BoxCollider& moving, const BodyFilter& filter, Vector3* push ) const {
	updateGrids();
	Vector3 candidate;
	bool found = false;
	Reach reach = reachOf( moving, Vector3( 0.f, 0.f, 0.f ) );
	mBoxGrid.collectArea( reach.low, reach.high, &mCandidates );
	for ( int index : mCandidates ) {
		if ( SweepMath::penetrateBox( moving, mBoxes[ index ], &candidate ) ) {
			keepDeeper( candidate, push, &found );
		}
	}
	mWallGrid.collectArea( reach.low, reach.high, &mCandidates );
	for ( int index : mCandidates ) {
		const WallMesh& walls = mWallMeshes[ index ];
		if ( !reach.touches( walls.boundsMin(), walls.boundsMax() ) ) {
			continue;
		}
		for ( const WallPiece& piece : walls.pieces() ) {
			if ( !reach.touches( piece.boundsMin, piece.boundsMax ) ) {
				continue;
			}
			PROFILE_COUNT( "Collide.walls", 1 ); // 細かく比べた壁のかけら
			if ( SweepMath::penetratePiece( moving, piece, &candidate ) ) {
				keepDeeper( candidate, push, &found );
			}
		}
	}
	if ( !filter.skipAllBodies ) {
		mBodyGrid.collectArea( reach.low, reach.high, &mCandidates );
		for ( int index : mCandidates ) {
			const ICollisionBody* body = mBodies[ index ];
			if ( filter.skips( body ) ) {
				continue;
			}
			PROFILE_COUNT( "Collide.bodies", 1 ); // 細かく比べた動く体
			if ( SweepMath::penetrateBox( moving, body->collider(), &candidate ) ) {
				keepDeeper( candidate, push, &found );
			}
		}
	}
	return found;
}
