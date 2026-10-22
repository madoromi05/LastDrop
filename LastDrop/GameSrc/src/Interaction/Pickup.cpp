#include "GameSrc/Interaction/Pickup.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Collision/CollisionWorld.h"

#include "GameLib/GameLib.h"

#include <string>

using namespace GameLib::Math;

Pickup::Pickup( const StaticMeshData* data, const Vector3& position, float angleY, ItemId item, int amount, bool blocksMovement ) :
	mBody( data, position, angleY ),
	mItem( item ),
	mAmount( amount ),
	mBlocksMovement( blocksMovement ),
	mTaken( false ) {
	ASSERT( amount > 0 && "Pickup : amount must be positive." );
}

void Pickup::draw( const ViewFrustum& frustum ) const {
	mBody.draw( frustum );
}

bool Pickup::isTaken() const {
	return mTaken;
}

bool Pickup::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mBody.raycast( origin, delta, time, normal );
}

bool Pickup::canInteract( const Inventory& inventory ) const {
	return !mTaken && inventory.canAdd( mItem );
}

std::string Pickup::prompt( const Inventory& inventory ) const {
	const std::string name = itemSpec( mItem ).name;
	if ( !inventory.canAdd( mItem ) ) {
		return "Can't carry more " + name;
	}
	// 1 つだけなら数は書かない（ガソリンの「Pick up Fuel」のまま）。
	if ( mAmount > 1 ) {
		return "[E] Pick up " + name + " (" + std::to_string( mAmount ) + ")";
	}
	return "[E] Pick up " + name;
}

void Pickup::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "Pickup::interact : cannot pick up." );
	while ( mAmount > 0 && inventory->canAdd( mItem ) ) {
		inventory->add( mItem );
		--mAmount;
	}
	mTaken = ( mAmount == 0 );
}

BoxCollider Pickup::collider() const {
	return mBody.collider();
}

void Pickup::addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	ASSERT( collisionWorld && hitWorld && "Pickup : world is null." );
	if ( mBlocksMovement ) {
		collisionWorld->addBody( this );
	}
	hitWorld->addBlocker( this );
}

void Pickup::removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	ASSERT( collisionWorld && hitWorld && "Pickup : world is null." );
	if ( mBlocksMovement ) {
		collisionWorld->removeBody( this );
	}
	hitWorld->removeBlocker( this );
}
