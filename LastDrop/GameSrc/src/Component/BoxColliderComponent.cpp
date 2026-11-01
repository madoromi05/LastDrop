#include "GameSrc/Component/BoxColliderComponent.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Collision/MeshBoundsBox.h"

#include "GameLib/GameLib.h"

BoxColliderComponent::BoxColliderComponent( Mode mode ) :
	mMode( mode ) {
}

void BoxColliderComponent::start() {
	// 見た目は start で置き場所へ動いている（StaticModelComponent を先に付けるので、先に start している）。
	mBox = MeshBoundsBox::make( requireComponent< StaticModelComponent >()->mesh() );
}

BoxCollider BoxColliderComponent::collider() const {
	return mBox;
}

void BoxColliderComponent::addTo( CollisionWorld* collisionWorld, HitWorld* ) {
	ASSERT( collisionWorld && "BoxColliderComponent : world is null." );
	switch ( mMode ) {
		case Mode::Fixed: collisionWorld->addBox( mBox ); break;
		case Mode::Removable: collisionWorld->addBody( this ); break;
		case Mode::WalkThrough: break;
	}
}

void BoxColliderComponent::removeFrom( CollisionWorld* collisionWorld, HitWorld* ) {
	ASSERT( collisionWorld && "BoxColliderComponent : world is null." );
	if ( mMode == Mode::Removable ) {
		collisionWorld->removeBody( this );
	}
}
