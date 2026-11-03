#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/MeshWallBuilder.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Render/DrawCulling.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Building::Building( const StaticMeshLods& shapes, const Vector3& position, float angleY, GroundMaterial material ) :
	mMesh( shapes.full(), position, angleY ),
	mShapes( shapes ),
	mMaterial( material ) {
}

void Building::addCollidersTo( CollisionWorld* world ) const {
	ASSERT( world && "Building : world is null." );
	world->addWallMesh( MeshWallBuilder::make( mMesh ) );
}

void Building::draw( const DrawCulling& culling ) const {
	if ( shows( culling ) ) {
		mMesh.draw( shapeFor( culling ) );
	}
}

bool Building::shows( const DrawCulling& culling ) const {
	return culling.shows( mMesh.boundsMin(), mMesh.boundsMax(), DrawCulling::Target::Large );
}

const StaticMeshInstance& Building::mesh() const {
	return mMesh;
}

const StaticMeshData* Building::shapeFor( const DrawCulling& culling ) const {
	return culling.chooseLod( mShapes, &mLod, mMesh.boundsMin(), mMesh.boundsMax() );
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
