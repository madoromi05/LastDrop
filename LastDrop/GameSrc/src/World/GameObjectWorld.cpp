#include "GameSrc/World/GameObjectWorld.h"

#include <algorithm>

namespace{
	const unsigned kFirstId = 1; // 0 は GameObjectHandle の「何も指さない」に使う

	template< class T >
	void eraseValue( std::vector< T >* values, const T& value ) {
		values->erase( std::remove( values->begin(), values->end(), value ), values->end() );
	}
}

GameObjectWorld::GameObjectWorld( CollisionWorld* collisionWorld, HitWorld* hitWorld ) :
	mCollisionWorld( collisionWorld ),
	mHitWorld( hitWorld ),
	mNextId( kFirstId ),
	mIterationDepth( 0 ) {
	ASSERT( mCollisionWorld && mHitWorld && "GameObjectWorld : world is null." );
}

GameObjectWorld::~GameObjectWorld() {
	// 後から作った物から消す（後の物が先の物を借りていることがあるため。自キャラが銃を借りる）。
	for ( auto it = mObjects.rbegin(); it != mObjects.rend(); ++it ) {
		if ( ( *it )->isSpawned() ) {
			detachAll( it->get() );
		}
	}
}

GameObject* GameObjectWorld::create( GameObjectTag tag, const char* name ) {
	ASSERT( name && "GameObjectWorld : name is null." );
	std::unique_ptr< GameObject > object = std::make_unique< GameObject >( this, tag, name, mNextId++ );
	GameObject* raw = object.get();
	mObjectsById[ raw->id() ] = raw;
	mObjects.push_back( std::move( object ) );
	return raw;
}

void GameObjectWorld::spawn( GameObject* object ) {
	ASSERT( object && object->world() == this && "GameObjectWorld : object is not mine." );
	ASSERT( !object->isSpawned() && "GameObjectWorld : object is already spawned." );
	ASSERT( mIterationDepth == 0 && "GameObjectWorld : spawn while iterating." );
	// 先に全部の部品の start を呼ぶ（隣の部品を探して覚えさせる）。名簿へ載せるのはその後（載せた途端に当たり判定を聞かれるため）。
	for ( int i = 0; i < object->componentNumber(); ++i ) {
		object->componentAt( i )->start();
	}
	for ( int i = 0; i < object->componentNumber(); ++i ) {
		attach( object, i );
	}
	object->markSpawned();
}

void GameObjectWorld::update( UpdatePhase phase ) {
	ASSERT( phase != UpdatePhase::None && "GameObjectWorld : phase None does not update." );
	IterationGuard guard( this );
	for ( Component* component : mPhaseComponents[ static_cast< int >( phase ) ] ) {
		component->update();
	}
}

void GameObjectWorld::destroyLater( const GameObject* object ) {
	if ( std::find( mPendingDestroy.begin(), mPendingDestroy.end(), object ) == mPendingDestroy.end() ) {
		mPendingDestroy.push_back( object );
	}
}

void GameObjectWorld::flush() {
	ASSERT( mIterationDepth == 0 && "GameObjectWorld : flush while iterating." );
	removeMarkedComponents();
	for ( const GameObject* doomed : mPendingDestroy ) {
		auto it = std::find_if( mObjects.begin(), mObjects.end(),
			[ doomed ]( const std::unique_ptr< GameObject >& o ) { return o.get() == doomed; } );
		if ( it == mObjects.end() ) {
			continue; // この持ち主の物ではない・もう消した
		}
		if ( ( *it )->isSpawned() ) {
			detachAll( it->get() );
		}
		mObjectsById.erase( ( *it )->id() );
		mObjects.erase( it );
	}
	mPendingDestroy.clear();
}

int GameObjectWorld::countTagged( GameObjectTag tag ) const {
	return static_cast< int >( std::count_if( mObjects.begin(), mObjects.end(),
		[ tag ]( const std::unique_ptr< GameObject >& o ) { return o->isSpawned() && o->tag() == tag; } ) );
}

GameObject* GameObjectWorld::oldestTagged( GameObjectTag tag ) const {
	for ( const std::unique_ptr< GameObject >& object : mObjects ) {
		if ( object->isSpawned() && object->tag() == tag ) {
			return object.get();
		}
	}
	return nullptr;
}

GameObject* GameObjectWorld::findById( unsigned id ) const {
	auto it = mObjectsById.find( id );
	return ( it == mObjectsById.end() ) ? nullptr : it->second;
}

void GameObjectWorld::attachLate( GameObject* object, int index ) {
	ASSERT( mIterationDepth == 0 && "GameObjectWorld : addComponent while iterating." );
	object->componentAt( index )->start();
	attach( object, index );
}

void GameObjectWorld::requestRemoval( GameObject* object ) {
	if ( std::find( mPendingRemoval.begin(), mPendingRemoval.end(), object ) == mPendingRemoval.end() ) {
		mPendingRemoval.push_back( object );
	}
}

const std::vector< GameObject* >* GameObjectWorld::objectsWith( int typeId ) const {
	if ( typeId >= static_cast< int >( mObjectsByType.size() ) ) {
		return nullptr;
	}
	return &mObjectsByType[ typeId ];
}

void GameObjectWorld::attach( GameObject* object, int index ) {
	if ( IWorldMember* member = object->worldMemberAt( index ) ) {
		member->addTo( mCollisionWorld, mHitWorld );
	}
	int typeId = object->componentTypeIdAt( index );
	if ( typeId >= static_cast< int >( mObjectsByType.size() ) ) {
		mObjectsByType.resize( typeId + 1 );
	}
	mObjectsByType[ typeId ].push_back( object );
	Component* component = object->componentAt( index );
	UpdatePhase phase = component->updatePhase();
	if ( phase != UpdatePhase::None ) {
		mPhaseComponents[ static_cast< int >( phase ) ].push_back( component );
	}
}

void GameObjectWorld::detach( GameObject* object, int index ) {
	Component* component = object->componentAt( index );
	component->onDestroy();
	if ( IWorldMember* member = object->worldMemberAt( index ) ) {
		member->removeFrom( mCollisionWorld, mHitWorld );
	}
	eraseValue( &mObjectsByType[ object->componentTypeIdAt( index ) ], object );
	UpdatePhase phase = component->updatePhase();
	if ( phase != UpdatePhase::None ) {
		eraseValue( &mPhaseComponents[ static_cast< int >( phase ) ], component );
	}
}

void GameObjectWorld::detachAll( GameObject* object ) {
	// 後に付けた部品は先に付けた部品を借りていることがあるので、逆の順に外す。
	for ( int i = object->componentNumber() - 1; i >= 0; --i ) {
		detach( object, i );
	}
}

void GameObjectWorld::removeMarkedComponents() {
	for ( GameObject* object : mPendingRemoval ) {
		// 消す印も付いている物は、物ごと消すときに外れる。まだ spawn していない物は、名簿に載っていないので捨てるだけ。
		for ( int i = object->componentNumber() - 1; i >= 0; --i ) {
			if ( !object->isRemovingAt( i ) ) {
				continue;
			}
			if ( object->isSpawned() ) {
				detach( object, i );
			}
			object->eraseAt( i );
		}
	}
	mPendingRemoval.clear();
}
