#include "GameSrc/Interaction/Inventory.h"

#include "GameLib/GameLib.h"
#include <algorithm>

namespace{
	const int kNoLimit = 0; // ItemSpec::carryLimit の「決めない」
}

Inventory::Inventory( const ItemGrid& bag ) :
	mGrid( bag ) {
}

bool Inventory::canAdd( ItemId id ) const {
	int limit = itemSpec( id ).carryLimit;
	if ( limit != kNoLimit && count( id ) >= limit ) {
		return false; // 2 丁目の銃（R2_Plan.txt Q14）
	}
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

int Inventory::countPlaceableInto( int cell, const ItemStack& stack ) const {
	int count = mGrid.countPlaceableInto( cell, stack );
	int limit = itemSpec( stack.item ).carryLimit;
	if ( limit != kNoLimit ) {
		count = std::min( count, std::max( limit - this->count( stack.item ), 0 ) ); // 2 丁目の銃は 0
	}
	return count;
}

void Inventory::placeInto( int cell, const ItemStack& stack, int count ) {
	ASSERT( count > 0 && count <= countPlaceableInto( cell, stack ) && "Inventory::placeInto : does not fit." );
	mGrid.placeInto( cell, stack, count );
}

void Inventory::setLoadedRounds( int owner, int rounds ) {
	mGrid.setLoadedRounds( owner, rounds );
}
