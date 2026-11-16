#include "GameSrc/Interaction/Item/MagazineSync.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

int magazineRoundsOf( const GameObjectWorld& objects, ItemId item ) {
	const GunComponent* gun = GunComponent::find( objects, item );
	return gun ? gun->magazineRounds() : 0;
}

void storeMagazineRounds( Inventory& inventory, const GameWorld& world ) {
	const ItemGrid& grid = inventory.grid();
	for ( int cell = 0; cell < grid.cellCount(); ++cell ) {
		if ( grid.ownerCell( cell ) != cell ) {
			continue;
		}
		ItemId item = grid.stackAt( cell ).item;
		if ( weaponForItem( item ) != WEAPON_NONE ) {
			inventory.setLoadedRounds( cell, magazineRoundsOf( world.objects(), item ) );
		}
	}
}

void restoreMagazineRounds( Inventory& inventory, GameWorld& world ) {
	for ( const ItemStack& stack : inventory.grid().stacks() ) {
		if ( weaponForItem( stack.item ) == WEAPON_NONE ) {
			continue;
		}
		if ( GunComponent* gun = GunComponent::find( world.objects(), stack.item ) ) {
			gun->setMagazineRounds( stack.loadedRounds );
		}
	}
}
