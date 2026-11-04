#include "GameSrc/Interaction/ItemTransfer.h"

#include "GameLib/GameLib.h"

namespace{
	const int kNoLimit = 0; // ItemSpec::carryLimit の「決めない」
}

namespace ItemTransfer{

bool satisfies( const ItemGrid& grid, GridRule rule ) {
	if ( rule == GridRule::Free ) {
		return true;
	}
	for ( int i = 0; i < ITEM_NUMBER; ++i ) {
		ItemId id = static_cast< ItemId >( i );
		int limit = itemSpec( id ).carryLimit;
		if ( limit != kNoLimit && grid.total( id ) > limit ) {
			return false;
		}
	}
	return true;
}

bool canMove( const ItemGrid& source, GridRule sourceRule, int from, const ItemGrid& target, GridRule targetRule, int to ) {
	if ( &source == &target ) {
		// 同じマス目の中の並べ替えは、物の数が変わらないので決まりは変わらない。
		return source.canMove( from, to );
	}
	ItemGrid sourceTrial = source;
	ItemGrid targetTrial = target;
	if ( !ItemGrid::canMoveBetween( sourceTrial, from, targetTrial, to ) ) {
		return false;
	}
	ItemGrid::moveBetween( &sourceTrial, from, &targetTrial, to );
	return satisfies( sourceTrial, sourceRule ) && satisfies( targetTrial, targetRule );
}

void move( ItemGrid* source, int from, ItemGrid* target, int to ) {
	ASSERT( source && target && "ItemTransfer::move : grid is null." );
	if ( source == target ) {
		source->move( from, to );
	} else {
		ItemGrid::moveBetween( source, from, target, to );
	}
}

} //namespace ItemTransfer
