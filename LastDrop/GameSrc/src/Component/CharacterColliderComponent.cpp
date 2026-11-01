#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

CharacterColliderComponent::CharacterColliderComponent() :
	mShape( nullptr ),
	mMover( this ), // this は覚えるだけ（長方形を読むのは動くとき＝作った後）
	mBlockingIn( nullptr ) {
}

void CharacterColliderComponent::awake() {
	mMover.setWorld( shared< CollisionWorld >() );
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
