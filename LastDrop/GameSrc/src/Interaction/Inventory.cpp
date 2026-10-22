#include "GameSrc/Interaction/Inventory.h"

#include "GameLib/GameLib.h"

Inventory::Inventory() {
	for ( int i = 0; i < ITEM_NUMBER; ++i ) {
		mCounts[ i ] = itemSpec( static_cast< ItemId >( i ) ).startCarry;
	}
}

bool Inventory::canAdd( ItemId id ) const {
	return count( id ) < itemSpec( id ).maxCarry;
}

void Inventory::add( ItemId id ) {
	ASSERT( canAdd( id ) && "Inventory::add : cannot carry more." );
	++mCounts[ id ];
}

void Inventory::remove( ItemId id ) {
	ASSERT( count( id ) > 0 && "Inventory::remove : not carried." );
	--mCounts[ id ];
}

int Inventory::count( ItemId id ) const {
	ASSERT( id >= 0 && id < ITEM_NUMBER && "Inventory : bad ItemId." );
	return mCounts[ id ];
}
