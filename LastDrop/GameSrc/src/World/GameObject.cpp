#include "GameSrc/World/GameObject.h"
#include "GameSrc/World/GameObjectWorld.h"

GameObject::GameObject( GameObjectWorld* world, GameObjectTag tag, const char* name, unsigned id ) :
	mWorld( world ),
	mTag( tag ),
	mName( name ),
	mId( id ),
	mSpawned( false ) {
	ASSERT( world && "GameObject : world is null." );
}

void GameObject::eraseAt( int index ) {
	mComponents.erase( mComponents.begin() + index );
}

void GameObject::attachLate( int index ) {
	mWorld->attachLate( this, index );
}

void GameObject::requestRemoval() {
	mWorld->requestRemoval( this );
}

