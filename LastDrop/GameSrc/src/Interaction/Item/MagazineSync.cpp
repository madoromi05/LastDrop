#include "GameSrc/Interaction/Item/MagazineSync.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

int magazineRoundsOf( const GameObjectWorld& objects, ItemId item ) {
	int rounds = 0;
	objects.forEach< GunComponent >( [ item, &rounds ]( const GunComponent& gun ) {
		if ( gun.spec().item == item ) {
			rounds = gun.magazineRounds();
		}
	} );
	return rounds;
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
		ItemId item = stack.item;
		int rounds = stack.loadedRounds;
		world.objects().forEach< GunComponent >( [ item, rounds ]( GunComponent& gun ) {
			if ( gun.spec().item == item ) {
				gun.setMagazineRounds( rounds );
			}
		} );
	}
}
