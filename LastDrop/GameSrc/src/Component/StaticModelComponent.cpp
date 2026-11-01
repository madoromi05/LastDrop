#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

StaticModelComponent::StaticModelComponent( const StaticMeshData* data, GroundMaterial material, DrawCulling::Target cullTarget ) :
	mMesh( data ),
	mMaterial( material ),
	mCullTarget( cullTarget ) {
}

void StaticModelComponent::start() {
	const TransformComponent* transform = requireComponent< TransformComponent >();
	mMesh.setPosition( transform->position() );
	mMesh.setAngleY( transform->angleY() );
}

void StaticModelComponent::draw( const DrawCulling& culling ) const {
	if ( culling.shows( mMesh.boundsMin(), mMesh.boundsMax(), mCullTarget ) ) {
		mMesh.draw();
	}
}

const StaticMeshInstance& StaticModelComponent::mesh() const {
	return mMesh;
}

bool StaticModelComponent::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mMesh.raycast( origin, delta, time, normal );
}

GroundMaterial StaticModelComponent::surfaceMaterial( const Vector3& ) const {
	return mMaterial;
}

void StaticModelComponent::addTo( CollisionWorld*, HitWorld* hitWorld ) {
	ASSERT( hitWorld && "StaticModelComponent : world is null." );
	hitWorld->addBlocker( this );
}

void StaticModelComponent::removeFrom( CollisionWorld*, HitWorld* hitWorld ) {
	ASSERT( hitWorld && "StaticModelComponent : world is null." );
	hitWorld->removeBlocker( this );
}
