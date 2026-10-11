#include "StaticMeshData.h"
#include "MoveHeight.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib::Math;

// 最初の頂点で必ず置き換わるよう、空の箱は min を最大・max を最小にしておく。
StaticMeshData::StaticMeshData() :
	mBoundsMin( FLOAT_MAX ),
	mBoundsMax( -FLOAT_MAX ),
	mBlockingBoundsMin( FLOAT_MAX ),
	mBlockingBoundsMax( -FLOAT_MAX ) {
}

StaticMeshData::~StaticMeshData() {
}

void StaticMeshData::create( int partNumber ) {
	mParts.setSize( partNumber );
}

int StaticMeshData::partNumber() const {
	return mParts.size();
}

StaticMeshPart& StaticMeshData::part( int index ) {
	return mParts[ index ];
}

bool StaticMeshData::isReady() const {
	if ( mParts.size() == 0 ) {
		return false;
	}
	for ( int i = 0; i < mParts.size(); ++i ) {
		if ( !mParts[ i ].isReady() ) {
			return false;
		}
	}
	return true;
}

void StaticMeshData::updateLoading() {
	for ( int i = 0; i < mParts.size(); ++i ) {
		mParts[ i ].updateLoading();
	}
}

void StaticMeshData::draw( const Matrix34& world ) const {
	for ( int i = 0; i < mParts.size(); ++i ) {
		mParts[ i ].draw( world );
	}
}

void StaticMeshData::appendTriangles( std::vector< Vector3 >* corners ) const {
	for ( int i = 0; i < mParts.size(); ++i ) {
		mParts[ i ].appendTriangles( corners );
	}
}

void StaticMeshData::extendBounds( const Vector3& position ) {
	mBoundsMin.min( position );
	mBoundsMax.max( position );
	if ( position.y > MoveHeight::kStepOver ) {
		mBlockingBoundsMin.min( position );
		mBlockingBoundsMax.max( position );
	}
}

const Vector3& StaticMeshData::boundsMin() const {
	return mBoundsMin;
}

const Vector3& StaticMeshData::boundsMax() const {
	return mBoundsMax;
}

const Vector3& StaticMeshData::blockingBoundsMin() const {
	return hasBlockingBounds() ? mBlockingBoundsMin : mBoundsMin;
}

const Vector3& StaticMeshData::blockingBoundsMax() const {
	return hasBlockingBounds() ? mBlockingBoundsMax : mBoundsMax;
}

bool StaticMeshData::hasBlockingBounds() const {
	return mBlockingBoundsMin.y <= mBlockingBoundsMax.y;
}

bool StaticMeshData::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	bool found = false;
	for ( int i = 0; i < mParts.size(); ++i ) {
		float t;
		Vector3 n;
		if ( !mParts[ i ].raycast( origin, delta, &t, &n ) ) {
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
