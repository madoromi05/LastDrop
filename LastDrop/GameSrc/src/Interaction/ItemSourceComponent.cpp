#include "GameSrc/Interaction/ItemSourceComponent.h"
#include "GameSrc/Interaction/ItemComponent.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"
#include <string>

ItemSourceComponent::ItemSourceComponent() :
	mItem( nullptr ) {
}

void ItemSourceComponent::start() {
	mItem = requireComponent< ItemComponent >();
}

bool ItemSourceComponent::canInteract( const Inventory& inventory ) const {
	return inventory.canAdd( mItem->item() );
}

std::string ItemSourceComponent::prompt( const Inventory& inventory ) const {
	const std::string name = itemSpec( mItem->item() ).name;
	if ( !inventory.canAdd( mItem->item() ) ) {
		return "Can't carry more " + name;
	}
	return "[E] Take " + name;
}

void ItemSourceComponent::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "ItemSourceComponent::interact : cannot take." );
	inventory->add( mItem->item() );
}
