#include "GameSrc/Interaction/ItemSource.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"
#include <string>

using namespace GameLib::Math;

namespace{
	// 置き場の見た目（今はガソリンの携行缶だけ）。違う物を置き場にするときは、作るときに受け取る形に変える。
	const GroundMaterial kBodyMaterial = GROUND_METAL;
}

ItemSource::ItemSource( const StaticMeshData* data, const Vector3& position, float angleY, ItemId item ) :
	mBody( data, position, angleY, kBodyMaterial ),
	mItem( item ) {
}

void ItemSource::addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) const {
	ASSERT( collisionWorld && hitWorld && "ItemSource : world is null." );
	mBody.addColliderTo( collisionWorld );
	hitWorld->addBlocker( &mBody );
}

void ItemSource::draw( const ViewFrustum& frustum ) const {
	mBody.draw( frustum );
}

bool ItemSource::isBody( const IRayBlocker* blocker ) const {
	return blocker == &mBody;
}

bool ItemSource::canInteract( const Inventory& inventory ) const {
	return inventory.canAdd( mItem );
}

std::string ItemSource::prompt( const Inventory& inventory ) const {
	const std::string name = itemSpec( mItem ).name;
	if ( !inventory.canAdd( mItem ) ) {
		return "Can't carry more " + name;
	}
	return "[E] Take " + name;
}

void ItemSource::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "ItemSource::interact : cannot take." );
	inventory->add( mItem );
}
