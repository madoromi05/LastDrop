#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/MeshWallBuilder.h"
#include "GameSrc/Collision/CollisionWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Building::Building( const StaticMeshData* data, const Vector3& position, float angleY, GroundMaterial material ) :
	mMesh( data, position, angleY ),
	mMaterial( material ) {
}

void Building::addCollidersTo( CollisionWorld* world ) const {
	ASSERT( world && "Building : world is null." );
	world->addWallMesh( MeshWallBuilder::make( mMesh ) );
}

void Building::draw( const ViewFrustum& frustum ) const {
	mMesh.draw( frustum );
}

const Vector3& Building::position() const {
	return mMesh.position();
}

bool Building::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mMesh.raycast( origin, delta, time, normal );
}

GroundMaterial Building::surfaceMaterial( const Vector3& ) const {
	return mMaterial;
}
