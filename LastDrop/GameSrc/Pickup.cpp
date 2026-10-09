#include "Pickup.h"
#include "Inventory.h"
#include "HitWorld.h"
#include "CollisionWorld.h"
#include "MeshBoundsBox.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Pickup::Pickup( const StaticMeshData* data, const Vector3& position, float angleY, ItemId item ) :
	mMesh( data ),
	mCollider( MeshBoundsBox::make( data, position, angleY ) ),
	mItem( item ),
	mTaken( false ) {
	mMesh.setPosition( position );
	mMesh.setAngleY( angleY );
}

void Pickup::draw() const {
	mMesh.draw();
}

bool Pickup::isTaken() const {
	return mTaken;
}

bool Pickup::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mMesh.raycast( origin, delta, time, normal );
}

bool Pickup::canInteract( const Inventory& inventory ) const {
	return !mTaken && inventory.canAdd( mItem );
}

std::string Pickup::prompt( const Inventory& inventory ) const {
	const std::string name = itemSpec( mItem ).name;
	if ( !inventory.canAdd( mItem ) ) {
		return "Can't carry more " + name;
	}
	return "[E] Pick up " + name;
}

void Pickup::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "Pickup::interact : cannot pick up." );
	inventory->add( mItem );
	mTaken = true;
}

BoxCollider Pickup::collider() const {
	return mCollider;
}

void Pickup::addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	ASSERT( collisionWorld && hitWorld && "Pickup : world is null." );
	collisionWorld->addBody( this );
	hitWorld->addBlocker( this );
}

void Pickup::removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	ASSERT( collisionWorld && hitWorld && "Pickup : world is null." );
	collisionWorld->removeBody( this );
	hitWorld->removeBlocker( this );
}
