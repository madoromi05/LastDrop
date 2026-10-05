#include "Building.h"
#include "StaticMeshData.h"
#include "CollisionWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
}

Building::Building( const StaticMeshData* data, const Vector3& position, float angleY ) :
	mMesh( data ) {
	ASSERT( data->isReady() && "Building : data is not ready." );
	mMesh.setPosition( position );
	mMesh.setAngleY( angleY );

	// 頂点を囲む箱（モデルのローカル）を、見た目と同じ置き場所・向きでワールドへ置く。
	const Vector3& low = data->boundsMin();
	const Vector3& high = data->boundsMax();
	Vector3 localCenter;
	localCenter.setAdd( low, high );
	localCenter *= kHalf;
	Matrix34 world; // StaticMeshInstance::draw と同じ順（移動 * 回転）
	world.setTranslation( position );
	world.rotateY( angleY );

	BoxCollider box;
	world.mul( &box.center, localCenter );
	box.halfWidthX = ( high.x - low.x ) * kHalf;
	box.halfWidthZ = ( high.z - low.z ) * kHalf;
	box.angleY = angleY;
	mColliders.push_back( box );
}

void Building::addCollidersTo( CollisionWorld* world ) const {
	ASSERT( world && "Building : world is null." );
	for ( const BoxCollider& box : mColliders ) {
		world->addBox( box );
	}
}

void Building::draw() const {
	mMesh.draw();
}

bool Building::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mMesh.raycast( origin, delta, time, normal );
}
