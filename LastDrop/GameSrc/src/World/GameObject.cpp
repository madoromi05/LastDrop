#include "GameSrc/World/GameObject.h"
#include "GameSrc/World/GameObjectWorld.h"

#include <algorithm>

GameObject::GameObject( GameObjectWorld* world, GameObjectTag tag, const char* name, unsigned id ) :
	mWorld( world ),
	mTag( tag ),
	mName( name ),
	mId( id ),
	mSpawned( false ) {
	ASSERT( world && "GameObject : world is null." );
}

bool GameObject::canRemoveMarked() const {
	for ( const Entry& entry : mComponents ) {
		if ( !entry.removing ) {
			continue;
		}
		for ( int requirerId : entry.requiredBy ) {
			const Entry* requirer = findEntry( requirerId );
			if ( requirer && !requirer->removing ) {
				return false; // 要った部品が残るのに、要られた部品だけ外そうとしている
			}
		}
	}
	return true;
}

void GameObject::eraseAt( int index ) {
	int erasedId = mComponents[ index ].typeId;
	mComponents.erase( mComponents.begin() + index );
	for ( Entry& entry : mComponents ) {
		entry.requiredBy.erase( std::remove( entry.requiredBy.begin(), entry.requiredBy.end(), erasedId ), entry.requiredBy.end() );
	}
}

void GameObject::markRequired( Entry* entry, const Component* requirer ) {
	if ( entry->component.get() == requirer ) {
		entry->locked = true;
		return;
	}
	auto found = std::find_if( mComponents.begin(), mComponents.end(),
		[ requirer ]( const Entry& e ) { return e.component.get() == requirer; } );
	ASSERT( found != mComponents.end() && "GameObject : the requirer is not attached to this object." );
	std::vector< int >& requiredBy = entry->requiredBy;
	if ( std::find( requiredBy.begin(), requiredBy.end(), found->typeId ) == requiredBy.end() ) {
		requiredBy.push_back( found->typeId );
	}
}

const GameObject::Entry* GameObject::findEntry( int typeId ) const {
	for ( const Entry& entry : mComponents ) {
		if ( entry.typeId == typeId ) {
			return &entry;
		}
	}
	return nullptr;
}

void GameObject::attachLate( int index ) {
	mWorld->attachLate( this, index );
}

void GameObject::requestRemoval() {
	mWorld->requestRemoval( this );
}
