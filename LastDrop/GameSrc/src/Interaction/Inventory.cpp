#include "GameSrc/Interaction/Inventory.h"

#include "GameLib/GameLib.h"

namespace{
	// 2 列にすると、2 マスの物（ガソリン）が 1 行にぴったり入り、強化 1 回で増える 2 マスも 1 行になる（GridInventory_Plan.txt Q1）。
	const int kColumns = 2;
	const int kStartRows = 4; // 始めは 8 マス（バイオハザード RE:2 と同じ）
}

Inventory::Inventory() :
	mGrid( kColumns, kStartRows ) {
	for ( int i = 0; i < ITEM_NUMBER; ++i ) {
		ItemId id = static_cast< ItemId >( i );
		for ( int n = 0; n < itemSpec( id ).startAmount; ++n ) {
			add( id );
		}
	}
}

bool Inventory::canAdd( ItemId id ) const {
	return mGrid.canAddOne( id );
}

void Inventory::add( ItemId id ) {
	ASSERT( canAdd( id ) && "Inventory::add : cannot carry more." );
	mGrid.addOne( id );
}

void Inventory::remove( ItemId id ) {
	ASSERT( count( id ) > 0 && "Inventory::remove : not carried." );
	mGrid.removeOne( id );
}

int Inventory::count( ItemId id ) const {
	ASSERT( id >= 0 && id < ITEM_NUMBER && "Inventory : bad ItemId." );
	return mGrid.total( id );
}

const ItemGrid& Inventory::grid() const {
	return mGrid;
}

bool Inventory::canMove( int from, int to ) const {
	return mGrid.canMove( from, to );
}

void Inventory::move( int from, int to ) {
	mGrid.move( from, to );
}

ItemStack Inventory::takeAll( int cell ) {
	return mGrid.takeAll( cell );
}

bool Inventory::canSetRows( int rows ) const {
	return mGrid.canSetRows( rows );
}

void Inventory::setRows( int rows ) {
	mGrid.setRows( rows );
}
