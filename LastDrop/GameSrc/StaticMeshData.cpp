#include "StaticMeshData.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib::Math;

// 最初の頂点で必ず置き換わるよう、空の箱は min を最大・max を最小にしておく。
StaticMeshData::StaticMeshData() :
	mBoundsMin( FLOAT_MAX ),
	mBoundsMax( -FLOAT_MAX ) {
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

void StaticMeshData::extendBounds( const Vector3& position ) {
	mBoundsMin.min( position );
	mBoundsMax.max( position );
}

const Vector3& StaticMeshData::boundsMin() const {
	return mBoundsMin;
}

const Vector3& StaticMeshData::boundsMax() const {
	return mBoundsMax;
}
