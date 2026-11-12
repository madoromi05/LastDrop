#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"
#include "GameSrc/Math/PlacementMatrix.h"
#include "GameSrc/Hit/HitMath.h"
#include "GameSrc/Render/Culling/ViewFrustum.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib::Math;

namespace{
	const float kNoRotation = 0.f;
	const float kActualSize = 1.f;
	const int kBoxCornerNumber = 8;
	// 隅の番号の 1・2・4 の位が、それぞれ X・Y・Z の小さい側／大きい側にあたる
	const int kCornerBitX = 1;
	const int kCornerBitY = 2;
	const int kCornerBitZ = 4;
	// 囲む箱を四方へ広げる幅（m）。箱はワールドで、三角形はモデルの空間で調べるので、
	// 部品の継ぎ目ちょうどを通る線分が、丸めの差でどちらの箱からも外れるのを防ぐ（外れを早く捨てるための箱なので、広い分には困らない）
	const float kBoundsPadding = 0.001f;
}

StaticMeshInstance::StaticMeshInstance( const StaticMeshData* data ) :
	StaticMeshInstance( data, Vector3( 0.f, 0.f, 0.f ), kNoRotation ) {
}

StaticMeshInstance::StaticMeshInstance( const StaticMeshData* data, const Vector3& position, float angleY ) :
	mData( data ),
	mPosition( position ),
	mAngleY( angleY ),
	mScale( kActualSize ) {
	ASSERT( mData && mData->isReady() && "StaticMeshInstance : data is null or not ready." );
	updatePlacement();
}

void StaticMeshInstance::setPosition( const Vector3& position ) {
	mPosition = position;
	updatePlacement();
}

void StaticMeshInstance::setAngleY( float degree ) {
	mAngleY = degree;
	updatePlacement();
}

void StaticMeshInstance::setScale( float scale ) {
	mScale = scale;
	updatePlacement();
}

const StaticMeshData* StaticMeshInstance::data() const {
	return mData;
}

const Vector3& StaticMeshInstance::position() const {
	return mPosition;
}

float StaticMeshInstance::angleY() const {
	return mAngleY;
}

float StaticMeshInstance::scale() const {
	return mScale;
}

const Matrix34& StaticMeshInstance::worldMatrix() const {
	return mWorld;
}

const Vector3& StaticMeshInstance::boundsMin() const {
	return mBoundsMin;
}

const Vector3& StaticMeshInstance::boundsMax() const {
	return mBoundsMax;
}

void StaticMeshInstance::updatePlacement() {
	mWorld = PlacementMatrix::make( mPosition, mAngleY, mScale );
	mToLocal.setInverse( mWorld );

	// モデルの空間の箱の8隅をワールドへ置き、それを囲む（回した箱は、軸に沿った箱からはみ出すため）
	const Vector3& low = mData->boundsMin();
	const Vector3& high = mData->boundsMax();
	for ( int i = 0; i < kBoxCornerNumber; ++i ) {
		Vector3 corner(
			( i & kCornerBitX ) ? high.x : low.x,
			( i & kCornerBitY ) ? high.y : low.y,
			( i & kCornerBitZ ) ? high.z : low.z );
		Vector3 placed;
		mWorld.mul( &placed, corner );
		if ( i == 0 ) {
			mBoundsMin = placed;
			mBoundsMax = placed;
		} else {
			mBoundsMin.min( placed );
			mBoundsMax.max( placed );
		}
	}
	const Vector3 padding( kBoundsPadding, kBoundsPadding, kBoundsPadding );
	mBoundsMin -= padding;
	mBoundsMax += padding;
}

void StaticMeshInstance::draw() const {
	mData->draw( mWorld );
}

void StaticMeshInstance::draw( const StaticMeshData* shape ) const {
	ASSERT( shape && "StaticMeshInstance : shape is null." );
	shape->draw( mWorld );
}

void StaticMeshInstance::draw( const ViewFrustum& frustum ) const {
	if ( frustum.mayContain( mBoundsMin, mBoundsMax ) ) {
		mData->draw( mWorld );
	}
}

bool StaticMeshInstance::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	if ( !HitMath::segmentTouchesBox( origin, delta, mBoundsMin, mBoundsMax ) ) {
		return false;
	}
	// 線分をモデルの空間へ移して調べる（移動・回転・拡大だけなので、線分上の時刻は移しても変わらない）
	Vector3 localOrigin;
	Vector3 localDelta;
	mToLocal.mul( &localOrigin, origin );
	mToLocal.mul33( &localDelta, delta );
	Vector3 localNormal;
	if ( !mData->raycast( localOrigin, localDelta, time, &localNormal ) ) {
		return false;
	}
	HitMath::normalToWorld( mToLocal, localNormal, normal );
	return true;
}
