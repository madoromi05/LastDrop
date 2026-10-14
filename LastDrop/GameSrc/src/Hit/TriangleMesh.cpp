#include "GameSrc/Hit/TriangleMesh.h"
#include "GameSrc/Hit/HitMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib::Math;

namespace{
	const int kVerticesPerTriangle = 3;
}

// 空のうちは min > max にしておき、どの線分も箱に触れないようにする。
TriangleMesh::TriangleMesh() :
	mBoundsMin( FLOAT_MAX ),
	mBoundsMax( -FLOAT_MAX ) {
}

void TriangleMesh::set( const std::vector< Vector3 >& positions, const std::vector< unsigned short >& indices ) {
	ASSERT( indices.size() % kVerticesPerTriangle == 0 && "TriangleMesh : index count must be a multiple of 3." );
	mPositions = positions;
	mIndices = indices;
	mBoundsMin.set( FLOAT_MAX, FLOAT_MAX, FLOAT_MAX );
	mBoundsMax.set( -FLOAT_MAX, -FLOAT_MAX, -FLOAT_MAX );
	for ( const Vector3& p : mPositions ) {
		mBoundsMin.min( p );
		mBoundsMax.max( p );
	}
}

bool TriangleMesh::isEmpty() const {
	return mIndices.empty();
}

void TriangleMesh::appendTriangles( std::vector< Vector3 >* corners ) const {
	for ( unsigned short index : mIndices ) {
		corners->push_back( mPositions[ index ] );
	}
}

bool TriangleMesh::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	if ( isEmpty() || !HitMath::segmentTouchesBox( origin, delta, mBoundsMin, mBoundsMax ) ) {
		return false;
	}
	bool found = false;
	for ( size_t i = 0; i < mIndices.size(); i += kVerticesPerTriangle ) {
		float t;
		Vector3 n;
		if ( !HitMath::segmentTriangle( origin, delta,
			mPositions[ mIndices[ i ] ], mPositions[ mIndices[ i + 1 ] ], mPositions[ mIndices[ i + 2 ] ], &t, &n ) ) {
			continue;
		}
		if ( found && t >= *time ) {
			continue;
		}
		*time = t;
		*normal = n;
		found = true;
	}
	return found;
}
