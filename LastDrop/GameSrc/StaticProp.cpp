#include "StaticProp.h"
#include "MeshBoundsBox.h"
#include "CollisionWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

StaticProp::StaticProp( const StaticMeshData* data, const Vector3& position, float angleY ) :
	mMesh( data ),
	mCollider( MeshBoundsBox::make( data, position, angleY ) ) {
	mMesh.setPosition( position );
	mMesh.setAngleY( angleY );
}

void StaticProp::addColliderTo( CollisionWorld* world ) const {
	ASSERT( world && "StaticProp : world is null." );
	world->addBox( mCollider );
}

void StaticProp::draw() const {
	mMesh.draw();
}

bool StaticProp::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mMesh.raycast( origin, delta, time, normal );
}
