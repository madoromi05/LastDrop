#include "GameSrc/Interaction/ItemGrid.h"

#include "GameLib/GameLib.h"
#include <algorithm>

namespace{
	const int kEmpty = -1;    // Cell::owner の空の印
	const int kNotFound = -1; // 置ける所・物が見つからない
	const ItemStack kNoStack = { ITEM_FUEL, 0 }; // 空のマスの中身（amount 0 が空。種類は使わない）
}

ItemGrid::ItemGrid( int columns, int rows ) :
	mColumns( columns ),
	mRows( rows ) {
	ASSERT( columns > 0 && rows > 0 && "ItemGrid : size must be positive." );
	Cell empty = { kEmpty, kNoStack };
	mCells.assign( mColumns * mRows, empty );
}

int ItemGrid::columns() const {
	return mColumns;
}

int ItemGrid::rows() const {
	return mRows;
}

int ItemGrid::cellCount() const {
	return static_cast< int >( mCells.size() );
}

int ItemGrid::ownerCell( int cell ) const {
	ASSERT( cell >= 0 && cell < cellCount() && "ItemGrid::ownerCell : bad cell." );
	return mCells[ cell ].owner;
}

const ItemStack& ItemGrid::stackAt( int owner ) const {
	ASSERT( owner >= 0 && owner < cellCount() && mCells[ owner ].owner == owner && "ItemGrid::stackAt : not an owner cell." );
	return mCells[ owner ].stack;
}

int ItemGrid::total( ItemId id ) const {
	int sum = 0;
	for ( int i = 0; i < cellCount(); ++i ) {
		if ( mCells[ i ].owner == i && mCells[ i ].stack.item == id ) {
			sum += mCells[ i ].stack.amount;
		}
	}
	return sum;
}

bool ItemGrid::canAddOne( ItemId id ) const {
	const ItemSpec& spec = itemSpec( id );
	for ( int i = 0; i < cellCount(); ++i ) {
		const Cell& cell = mCells[ i ];
		if ( cell.owner == i && cell.stack.item == id && cell.stack.amount < spec.stackSize ) {
			return true;
		}
	}
	return firstFree( mCells, spec.cellSize ) != kNotFound;
}

void ItemGrid::addOne( ItemId id ) {
	ASSERT( canAddOne( id ) && "ItemGrid::addOne : no room." );
	const ItemSpec& spec = itemSpec( id );
	for ( int i = 0; i < cellCount(); ++i ) {
		Cell& cell = mCells[ i ];
		if ( cell.owner == i && cell.stack.item == id && cell.stack.amount < spec.stackSize ) {
			++cell.stack.amount;
			return;
		}
	}
	ItemStack one = { id, 1 };
	place( &mCells, firstFree( mCells, spec.cellSize ), one );
}

void ItemGrid::removeOne( ItemId id ) {
	int smallest = kNotFound;
	for ( int i = 0; i < cellCount(); ++i ) {
		const Cell& cell = mCells[ i ];
		if ( cell.owner != i || cell.stack.item != id ) {
			continue;
		}
		// <= にして、同じ数なら後ろ（下）のマスを選ぶ。
		if ( smallest == kNotFound || cell.stack.amount <= mCells[ smallest ].stack.amount ) {
			smallest = i;
		}
	}
	ASSERT( smallest != kNotFound && "ItemGrid::removeOne : not carried." );
	--mCells[ smallest ].stack.amount;
	if ( mCells[ smallest ].stack.amount == 0 ) {
		clear( &mCells, smallest );
	}
}

ItemStack ItemGrid::takeAll( int cell ) {
	int owner = ownerCell( cell );
	ASSERT( owner != kEmpty && "ItemGrid::takeAll : empty cell." );
	ItemStack taken = mCells[ owner ].stack;
	clear( &mCells, owner );
	return taken;
}

int ItemGrid::placementCell( ItemId id, int cell ) const {
	ASSERT( cell >= 0 && cell < cellCount() && "ItemGrid::placementCell : bad cell." );
	int row = cell / mColumns;
	int column = std::min( cell % mColumns, mColumns - itemSpec( id ).cellSize );
	return row * mColumns + std::max( column, 0 );
}

bool ItemGrid::canMove( int from, int to ) const {
	Cells trial = mCells;
	return applyMove( &trial, from, to );
}

void ItemGrid::move( int from, int to ) {
	Cells trial = mCells;
	bool moved = applyMove( &trial, from, to );
	ASSERT( moved && "ItemGrid::move : cannot move (check canMove first)." );
	if ( moved ) {
		mCells.swap( trial );
	}
}

bool ItemGrid::canSetRows( int rows ) const {
	if ( rows <= 0 ) {
		return false;
	}
	for ( int i = rows * mColumns; i < cellCount(); ++i ) {
		if ( mCells[ i ].owner != kEmpty ) {
			return false;
		}
	}
	return true;
}

void ItemGrid::setRows( int rows ) {
	ASSERT( canSetRows( rows ) && "ItemGrid::setRows : items would be lost." );
	Cell empty = { kEmpty, kNoStack };
	mCells.resize( rows * mColumns, empty );
	mRows = rows;
}

bool ItemGrid::isInside( int row, int column, int size ) const {
	return row >= 0 && row < mRows && column >= 0 && column + size <= mColumns;
}

bool ItemGrid::isFree( const Cells& cells, int owner, int size ) const {
	if ( !isInside( owner / mColumns, owner % mColumns, size ) ) {
		return false;
	}
	for ( int i = 0; i < size; ++i ) {
		if ( cells[ owner + i ].owner != kEmpty ) {
			return false;
		}
	}
	return true;
}

int ItemGrid::firstFree( const Cells& cells, int size ) const {
	for ( int i = 0; i < static_cast< int >( cells.size() ); ++i ) {
		if ( isFree( cells, i, size ) ) {
			return i;
		}
	}
	return kNotFound;
}

void ItemGrid::place( Cells* cells, int owner, const ItemStack& stack ) {
	int size = itemSpec( stack.item ).cellSize;
	for ( int i = 0; i < size; ++i ) {
		( *cells )[ owner + i ].owner = owner;
	}
	( *cells )[ owner ].stack = stack;
}

void ItemGrid::clear( Cells* cells, int owner ) {
	int size = itemSpec( ( *cells )[ owner ].stack.item ).cellSize;
	for ( int i = 0; i < size; ++i ) {
		( *cells )[ owner + i ].owner = kEmpty;
		( *cells )[ owner + i ].stack = kNoStack;
	}
}

bool ItemGrid::applyMove( Cells* cells, int from, int to ) const {
	int source = ( *cells )[ from ].owner;
	ASSERT( source != kEmpty && "ItemGrid::applyMove : nothing to move." );
	ItemStack moving = ( *cells )[ source ].stack;
	const ItemSpec& spec = itemSpec( moving.item );
	int target = placementCell( moving.item, to );
	if ( target == source ) {
		return false; // 同じ所（動かさない。画面では取りやめとして扱う）
	}

	// 置く先のマスを使っている、動かす物とは別の物（2 マスの物を置く先に 1 マスの物が 2 つあることもある）。
	std::vector< int > others;
	for ( int i = 0; i < spec.cellSize; ++i ) {
		int other = ( *cells )[ target + i ].owner;
		if ( other != kEmpty && other != source && std::find( others.begin(), others.end(), other ) == others.end() ) {
			others.push_back( other );
		}
	}

	// ① 同じ物で重ねられるなら、入る分だけ足す（入りきらない分は元のマスに残る）。
	if ( others.size() == 1 ) {
		Cell& into = ( *cells )[ others[ 0 ] ];
		if ( into.stack.item == moving.item && spec.stackSize > 1 && into.stack.amount < spec.stackSize ) {
			int count = std::min( moving.amount, spec.stackSize - into.stack.amount );
			into.stack.amount += count;
			( *cells )[ source ].stack.amount -= count;
			if ( ( *cells )[ source ].stack.amount == 0 ) {
				clear( cells, source );
			}
			return true;
		}
	}

	// ② 入れ替え。動かす物と、置く先の物をいったん全部どけてから置き直す。
	std::vector< ItemStack > otherStacks;
	for ( int other : others ) {
		otherStacks.push_back( ( *cells )[ other ].stack );
		clear( cells, other );
	}
	clear( cells, source );
	if ( !isFree( *cells, target, spec.cellSize ) ) {
		return false;
	}
	place( cells, target, moving );
	// どけた物は、置く先と同じ並び（ずれ）のまま元の場所へ。入らなければ、上の行・左の列から空いている所へ。
	for ( size_t i = 0; i < others.size(); ++i ) {
		int size = itemSpec( otherStacks[ i ].item ).cellSize;
		int row = source / mColumns + ( others[ i ] / mColumns - target / mColumns );
		int column = source % mColumns + ( others[ i ] % mColumns - target % mColumns );
		int owner = kNotFound;
		if ( isInside( row, column, size ) && isFree( *cells, row * mColumns + column, size ) ) {
			owner = row * mColumns + column;
		} else {
			owner = firstFree( *cells, size );
		}
		if ( owner == kNotFound ) {
			return false;
		}
		place( cells, owner, otherStacks[ i ] );
	}
	return true;
}
