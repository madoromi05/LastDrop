#include "GameSrc/Interaction/Item/InventoryComponent.h"
#include "GameSrc/World/GameObject.h"

InventoryComponent::InventoryComponent( const ItemGrid& bag ) :
	mInventory( bag ) {
}

Inventory& InventoryComponent::inventory() {
	return mInventory;
}

Inventory* InventoryComponent::inventoryOf( GameObject* holder ) {
	InventoryComponent* component = holder ? holder->getComponent< InventoryComponent >() : nullptr;
	return component ? &component->inventory() : nullptr;
}
