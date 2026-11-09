#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

StaticModelComponent::StaticModelComponent( const StaticMeshData* data, GroundMaterial material, DrawCulling::Target cullTarget ) :
	StaticModelComponent( StaticMeshLods( data ), material, cullTarget ) {
}

StaticModelComponent::StaticModelComponent( const StaticMeshLods& shapes, GroundMaterial material, DrawCulling::Target cullTarget ) :
	mMesh( shapes.full() ),
	mShapes( shapes ),
	mMaterial( material ),
	mCullTarget( cullTarget ) {
}

void StaticModelComponent::awake() {
	const TransformComponent* transform = requireComponent< TransformComponent >();
	mMesh.setPosition( transform->position() );
	mMesh.setAngleY( transform->angleY() );
}

void StaticModelComponent::draw( const DrawCulling& culling ) const {
	if ( culling.shows( mMesh.boundsMin(), mMesh.boundsMax(), mCullTarget ) ) {
		mMesh.draw( culling.chooseLod( mShapes, &mLod, mMesh.boundsMin(), mMesh.boundsMax() ) );
	}
}

const StaticMeshInstance& StaticModelComponent::mesh() const {
	return mMesh;
}

void StaticModelComponent::setShapes( const StaticMeshLods& shapes ) {
	ASSERT( shapes.full() && shapes.full()->isReady() && "StaticModelComponent::setShapes : shapes must be ready." );
	mShapes = shapes;
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
