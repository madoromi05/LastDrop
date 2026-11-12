#include "GameSrc/Collision/WallMesh.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib::Math;

// 最初のかけらで必ず置き換わるよう、空の範囲は min を最大・max を最小にしておく
WallMesh::WallMesh() :
	mBoundsMin( FLOAT_MAX ),
	mBoundsMax( -FLOAT_MAX ) {
}

void WallMesh::add( const WallPiece& piece ) {
	mPieces.push_back( piece );
	mBoundsMin.min( piece.boundsMin );
	mBoundsMax.max( piece.boundsMax );
}

const std::vector< WallPiece >& WallMesh::pieces() const {
	return mPieces;
}

const Vector3& WallMesh::boundsMin() const {
	return mBoundsMin;
}

const Vector3& WallMesh::boundsMax() const {
	return mBoundsMax;
}
