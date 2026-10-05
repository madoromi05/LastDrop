#include "StaticMeshInstance.h"
#include "StaticMeshData.h"
#include "HitMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib::Math;

StaticMeshInstance::StaticMeshInstance( const StaticMeshData* data ) :
	mData( data ),
	mPosition( 0.f, 0.f, 0.f ),
	mAngleY( 0.f ),
	mScale( 1.f ) {
	ASSERT( mData && "StaticMeshInstance : data is null." );
}

void StaticMeshInstance::setPosition( const Vector3& position ) {
	mPosition = position;
}

void StaticMeshInstance::setAngleY( float degree ) {
	mAngleY = degree;
}

void StaticMeshInstance::setScale( float scale ) {
	mScale = scale;
}

Matrix34 StaticMeshInstance::worldMatrix() const {
	// world = 移動 * 回転 * 拡大（拡大→回転→移動の順に効く）。
	Matrix34 world;
	world.setTranslation( mPosition );
	world.rotateY( mAngleY );
	world.scale( mScale );
	return world;
}

void StaticMeshInstance::draw() const {
	mData->draw( worldMatrix() );
}

bool StaticMeshInstance::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	// 線分をモデルの空間へ移して調べる（移動・回転・拡大だけなので、線分上の時刻は移しても変わらない）。
	Matrix34 toLocal;
	toLocal.setInverse( worldMatrix() );
	Vector3 localOrigin;
	Vector3 localDelta;
	toLocal.mul( &localOrigin, origin );
	toLocal.mul33( &localDelta, delta );
	Vector3 localNormal;
	if ( !mData->raycast( localOrigin, localDelta, time, &localNormal ) ) {
		return false;
	}
	HitMath::normalToWorld( toLocal, localNormal, normal );
	return true;
}
