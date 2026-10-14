#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/MeshWallBuilder.h"
#include "GameSrc/Collision/CollisionWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Building::Building( const StaticMeshData* data, const Vector3& position, float angleY ) :
	mMesh( data, position, angleY ) {
}

void Building::addCollidersTo( CollisionWorld* world ) const {
	ASSERT( world && "Building : world is null." );
	world->addWallMesh( MeshWallBuilder::make( mMesh ) );
}

void Building::draw( const ViewFrustum& frustum ) const {
	mMesh.draw( frustum );
}

bool Building::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mMesh.raycast( origin, delta, time, normal );
}
