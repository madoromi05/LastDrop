#include "Building.h"
#include "MeshBoundsBox.h"
#include "CollisionWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Building::Building( const StaticMeshData* data, const Vector3& position, float angleY ) :
	mMesh( data ) {
	mMesh.setPosition( position );
	mMesh.setAngleY( angleY );

	mColliders.push_back( MeshBoundsBox::make( data, position, angleY ) );
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
