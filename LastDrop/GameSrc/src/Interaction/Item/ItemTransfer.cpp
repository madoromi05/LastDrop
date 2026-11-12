#include "GameSrc/Interaction/Item/ItemTransfer.h"

#include "GameLib/GameLib.h"

namespace{
	const int kNoLimit = 0; // ItemSpec::carryLimit の「決めない」
	const int kNoCell = -1; // ItemGrid::ownerCell の「空のマス」
}

namespace ItemTransfer{

bool satisfies( const ItemGrid& grid, GridRule rule ) {
	if ( rule == GridRule::Free ) {
		return true;
	}
	if ( rule == GridRule::FuelOnly ) {
		for ( const ItemStack& stack : grid.stacks() ) {
			if ( stack.item != ITEM_FUEL ) {
				return false;
			}
		}
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
		// 同じマス目の中の並べ替えは、物の数が変わらないので決まりは変わらない
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

bool quickMove( ItemGrid* source, GridRule sourceRule, int from, ItemGrid* target, GridRule targetRule ) {
	ASSERT( source && target && source != target && "ItemTransfer::quickMove : bad grids." );
	int owner = source->ownerCell( from );
	if ( owner == kNoCell ) {
		return false;
	}
	ItemId item = source->stackAt( owner ).item;
	bool movedAny = false;
	// 一度に動くのは 1 マスぶんまで（重ねたマスが一杯になったら、次のマスを探し直す）。マスの数より多くは回らない
	for ( int pass = 0; pass <= target->cellCount() && source->ownerCell( owner ) == owner; ++pass ) {
		int to = kNoCell;
		// ① 同じ物で重ねられるマス
		for ( int cell = 0; cell < target->cellCount() && to == kNoCell; ++cell ) {
			if ( target->ownerCell( cell ) != cell ) {
				continue;
			}
			const ItemStack& there = target->stackAt( cell );
			if ( there.item == item && there.amount < itemSpec( item ).stackSize
				&& canMove( *source, sourceRule, owner, *target, targetRule, cell ) ) {
				to = cell;
			}
		}
		// ② 空き（物の大きさの分だけ空いている所。置いた先に物があると入れ替えになるので、空いている所だけ）
		for ( int cell = 0; cell < target->cellCount() && to == kNoCell; ++cell ) {
			if ( target->countPlaceableInto( cell, source->stackAt( owner ) ) > 0 && target->ownerCell( cell ) == kNoCell
				&& target->placementCell( item, cell ) == cell && canMove( *source, sourceRule, owner, *target, targetRule, cell ) ) {
				to = cell;
			}
		}
		if ( to == kNoCell ) {
			break;
		}
		ItemGrid::moveBetween( source, owner, target, to );
		movedAny = true;
	}
	return movedAny;
}

} //namespace ItemTransfer
