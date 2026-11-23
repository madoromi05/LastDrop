#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Component/TransformComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Render/StaticMesh/StaticMeshData.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

StaticModelComponent::StaticModelComponent( const StaticMeshLods& shapes, GroundMaterial material, DrawCulling::Target cullTarget ) :
	mMesh( shapes.full() ),
	mShapes( shapes ),
	mMaterial( material ),
	mCullTarget( cullTarget ),
	mTransform( nullptr ),
	mHidden( false ) {
}

void StaticModelComponent::awake() {
	mTransform = requireComponent< TransformComponent >();
	followTransform();
}

void StaticModelComponent::followTransform() {
	mMesh.setPosition( mTransform->position() );
	mMesh.setAngleY( mTransform->angleY() );
}

void StaticModelComponent::setHidden( bool hidden ) {
	mHidden = hidden;
}

bool StaticModelComponent::isHidden() const {
	return mHidden;
}

void StaticModelComponent::draw( const DrawCulling& culling ) const {
	if ( shows( culling ) ) {
		mMesh.draw( shapeFor( culling ) );
	}
}

bool StaticModelComponent::shows( const DrawCulling& culling ) const {
	return !mHidden && culling.shows( mMesh.boundsMin(), mMesh.boundsMax(), mCullTarget );
}

const StaticMeshData* StaticModelComponent::shapeFor( const DrawCulling& culling ) const {
	return culling.chooseLod( mShapes, &mLod, mMesh.boundsMin(), mMesh.boundsMax() );
}

const StaticMeshInstance& StaticModelComponent::mesh() const {
	return mMesh;
}

void StaticModelComponent::setShapes( const StaticMeshLods& shapes ) {
	ASSERT( shapes.full() && shapes.full()->isReady() && "StaticModelComponent::setShapes : shapes must be ready." );
	mShapes = shapes;
}

bool StaticModelComponent::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return !mHidden && mMesh.raycast( origin, delta, time, normal );
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
