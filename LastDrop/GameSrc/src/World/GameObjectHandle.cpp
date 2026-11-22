#include "GameSrc/World/GameObjectHandle.h"
#include "GameSrc/World/GameObjectWorld.h"

GameObjectHandle::GameObjectHandle() :
	mWorld( nullptr ),
	mId( 0 ) {
}

GameObjectHandle::GameObjectHandle( const GameObject* object ) :
	mWorld( object ? object->world() : nullptr ),
	mId( object ? object->id() : 0 ) {
}

GameObject* GameObjectHandle::get() const {
	return mWorld ? mWorld->findById( mId ) : nullptr;
}
