#include "GameSrc/Interaction/Item/MagazineSync.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

void storeMagazineRounds( Inventory& inventory, const GameWorld& world ) {
	const ItemGrid& grid = inventory.grid();
	for ( int cell = 0; cell < grid.cellCount(); ++cell ) {
		if ( grid.ownerCell( cell ) != cell ) {
			continue;
		}
		ItemId item = grid.stackAt( cell ).item;
		if ( weaponForItem( item ) != WEAPON_NONE ) {
			inventory.setLoadedRounds( cell, world.magazineRounds( item ) );
		}
	}
}

void restoreMagazineRounds( Inventory& inventory, GameWorld& world ) {
	for ( const ItemStack& stack : inventory.grid().stacks() ) {
		if ( weaponForItem( stack.item ) != WEAPON_NONE ) {
			world.setMagazineRounds( stack.item, stack.loadedRounds );
		}
	}
}
