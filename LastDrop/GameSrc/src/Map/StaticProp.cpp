#include "GameSrc/Map/StaticProp.h"
#include "GameSrc/Collision/MeshBoundsBox.h"
#include "GameSrc/Collision/CollisionWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

StaticProp::StaticProp( const StaticMeshData* data, const Vector3& position, float angleY, GroundMaterial material ) :
	mMesh( data, position, angleY ),
	mCollider( MeshBoundsBox::make( mMesh ) ), // mMesh を先に置いてから作る（メンバの宣言順）
	mMaterial( material ) {
}

void StaticProp::addColliderTo( CollisionWorld* world ) const {
	ASSERT( world && "StaticProp : world is null." );
	world->addBox( mCollider );
}

const BoxCollider& StaticProp::collider() const {
	return mCollider;
}

void StaticProp::draw( const DrawCulling& culling, DrawCulling::Target target ) const {
	if ( culling.shows( mMesh.boundsMin(), mMesh.boundsMax(), target ) ) {
		mMesh.draw();
	}
}

bool StaticProp::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mMesh.raycast( origin, delta, time, normal );
}

GroundMaterial StaticProp::surfaceMaterial( const Vector3& ) const {
	return mMaterial;
}
