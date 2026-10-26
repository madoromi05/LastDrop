#include "GameSrc/World/GameObjectWorld.h"

#include <algorithm>

GameObjectWorld::GameObjectWorld( CollisionWorld* collisionWorld, HitWorld* hitWorld ) :
	mCollisionWorld( collisionWorld ),
	mHitWorld( hitWorld ),
	mIterationDepth( 0 ) {
	ASSERT( mCollisionWorld && mHitWorld && "GameObjectWorld : world is null." );
}

GameObjectWorld::~GameObjectWorld() {
	for ( std::unique_ptr< GameObject >& object : mObjects ) {
		detach( object.get() );
	}
}

GameObject* GameObjectWorld::spawn( std::unique_ptr< GameObject > object ) {
	ASSERT( object && "GameObjectWorld : object is null." );
	ASSERT( mIterationDepth == 0 && "GameObjectWorld : spawn while iterating." );
	GameObject* raw = object.get();
	for ( int i = 0; i < raw->componentNumber(); ++i ) {
		if ( IWorldMember* member = raw->worldMemberAt( i ) ) {
			member->addTo( mCollisionWorld, mHitWorld );
		}
		int typeId = raw->componentTypeIdAt( i );
		if ( typeId >= static_cast< int >( mObjectsByType.size() ) ) {
			mObjectsByType.resize( typeId + 1 );
		}
		mObjectsByType[ typeId ].push_back( raw );
	}
	mObjects.push_back( std::move( object ) );
	return raw;
}

void GameObjectWorld::destroyLater( const GameObject* object ) {
	if ( std::find( mPendingDestroy.begin(), mPendingDestroy.end(), object ) == mPendingDestroy.end() ) {
		mPendingDestroy.push_back( object );
	}
}

void GameObjectWorld::flush() {
	ASSERT( mIterationDepth == 0 && "GameObjectWorld : flush while iterating." );
	for ( const GameObject* doomed : mPendingDestroy ) {
		auto it = std::find_if( mObjects.begin(), mObjects.end(),
			[ doomed ]( const std::unique_ptr< GameObject >& o ) { return o.get() == doomed; } );
		if ( it == mObjects.end() ) {
			continue; // この持ち主の物ではない・もう消した
		}
		detach( it->get() );
		mObjects.erase( it );
	}
	mPendingDestroy.clear();
}

int GameObjectWorld::countTagged( GameObjectTag tag ) const {
	return static_cast< int >( std::count_if( mObjects.begin(), mObjects.end(),
		[ tag ]( const std::unique_ptr< GameObject >& o ) { return o->tag() == tag; } ) );
}

GameObject* GameObjectWorld::oldestTagged( GameObjectTag tag ) const {
	for ( const std::unique_ptr< GameObject >& object : mObjects ) {
		if ( object->tag() == tag ) {
			return object.get();
		}
	}
	return nullptr;
}

const std::vector< GameObject* >* GameObjectWorld::objectsWith( int typeId ) const {
	if ( typeId >= static_cast< int >( mObjectsByType.size() ) ) {
		return nullptr;
	}
	return &mObjectsByType[ typeId ];
}

void GameObjectWorld::detach( GameObject* object ) {
	for ( int i = 0; i < object->componentNumber(); ++i ) {
		if ( IWorldMember* member = object->worldMemberAt( i ) ) {
			member->removeFrom( mCollisionWorld, mHitWorld );
		}
		std::vector< GameObject* >& sameType = mObjectsByType[ object->componentTypeIdAt( i ) ];
		sameType.erase( std::remove( sameType.begin(), sameType.end(), object ), sameType.end() );
	}
}
