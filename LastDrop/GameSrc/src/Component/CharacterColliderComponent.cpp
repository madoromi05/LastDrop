#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Collision/CollisionWorld.h"

#include "GameLib/GameLib.h"

CharacterColliderComponent::CharacterColliderComponent( const CollisionWorld* world ) :
	mShape( nullptr ),
	mMover( world, this ), // this は覚えるだけ（長方形を読むのは動くとき＝作った後）
	mBlockingIn( nullptr ) {
	ASSERT( world && "CharacterColliderComponent : world is null." );
}

void CharacterColliderComponent::setShape( const ICollisionBody* shape ) {
	ASSERT( shape && !mShape && "CharacterColliderComponent : shape is null or already set." );
	mShape = shape;
}

const CharacterMover& CharacterColliderComponent::mover() const {
	return mMover;
}

void CharacterColliderComponent::stopBlocking() {
	if ( mBlockingIn ) {
		mBlockingIn->removeBody( this );
		mBlockingIn = nullptr;
	}
}

bool CharacterColliderComponent::isBlocking() const {
	return mBlockingIn != nullptr;
}

BoxCollider CharacterColliderComponent::collider() const {
	ASSERT( mShape && "CharacterColliderComponent : shape is not set." );
	return mShape->collider();
}

void CharacterColliderComponent::addTo( CollisionWorld* collisionWorld, HitWorld* ) {
	ASSERT( collisionWorld && "CharacterColliderComponent : world is null." );
	collisionWorld->addBody( this );
	mBlockingIn = collisionWorld;
}

void CharacterColliderComponent::removeFrom( CollisionWorld*, HitWorld* ) {
	stopBlocking(); // もう外れていれば何もしない
}
