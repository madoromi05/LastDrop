#include "GameSrc/Scene/Game/Screen/InventoryCursor.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

#include "GameLib/GameLib.h"
#include <algorithm>

InventoryCursor::InventoryCursor() :
	mMode( Mode::Browse ),
	mSelected( 0 ),
	mMenuCount( 0 ),
	mMenuChoice( 0 ),
	mMovingFrom( kNoCell ) {
	for ( MenuItem& item : mMenuItems ) {
		item = MENU_CANCEL;
	}
}

void InventoryCursor::open() {
	mMode = Mode::Browse;
	mSelected = 0;
	mMenuCount = 0;
	mMenuChoice = 0;
	mMovingFrom = kNoCell;
}

void InventoryCursor::openForPlacing( const Inventory& inventory, const ItemStack& incoming ) {
	mMode = Mode::Placing;
	mSelected = 0;
	for ( int cell = 0; cell < inventory.grid().cellCount(); ++cell ) {
		if ( inventory.countPlaceableInto( cell, incoming ) > 0 ) {
			mSelected = cell; // 入る最初のマス（決定を 1 回押すだけで入れられるように）
			break;
		}
	}
}

bool InventoryCursor::back() {
	if ( mMode == Mode::Browse ) {
		return false;
	}
	mMode = Mode::Browse;
	mMovingFrom = kNoCell;
	return true;
}

void InventoryCursor::forgetStale( const ItemGrid& grid ) {
	// F6 で行を減らした・物を足したときに、消えたマスや、もう無い物を指したままにしない。
	mSelected = std::min( mSelected, grid.cellCount() - 1 );
	if ( mMode == Mode::Moving && ( mMovingFrom >= grid.cellCount() || grid.ownerCell( mMovingFrom ) != mMovingFrom ) ) {
		back();
	}
	if ( mMode == Mode::Menu && grid.ownerCell( mSelected ) == kNoCell ) {
		back();
	}
}

void InventoryCursor::endPlacingIfEmpty( const ItemStack* incoming ) {
	if ( mMode == Mode::Placing && !incoming ) {
		mMode = Mode::Browse; // 拾う物が無くなった（全部入れた）
	}
}

void InventoryCursor::selectCell( int cell ) {
	if ( cell != kNoCell ) {
		mSelected = cell;
	}
}

void InventoryCursor::moveSelection( bool up, bool down, bool left, bool right, const ItemGrid& grid ) {
	int row = mSelected / grid.columns();
	int column = mSelected % grid.columns();
	if ( up ) {
		row = std::max( row - 1, 0 );
	}
	if ( down ) {
		row = std::min( row + 1, grid.rows() - 1 );
	}
	if ( left ) {
		column = std::max( column - 1, 0 );
	}
	if ( right ) {
		column = std::min( column + 1, grid.columns() - 1 );
	}
	mSelected = row * grid.columns() + column;
}

InventoryCursor::Request InventoryCursor::decide( Inventory* inventory, ItemId equipped, bool injured, const ItemStack* incoming ) {
	const Request nothing = { Request::NONE, kNoCell };
	ASSERT( inventory && "InventoryCursor::decide : inventory is null." );
	if ( mMode == Mode::Placing ) {
		// 入らない所なら何もしない（赤い枠のまま、別の所を選んでもらう）。入れるのは本編（拾う物から減らすのも本編）。
		return ( incoming && inventory->countPlaceableInto( mSelected, *incoming ) > 0 ) ? Request{ Request::PLACE, mSelected } : nothing;
	}
	if ( mMode == Mode::Moving ) {
		place( inventory );
	} else if ( inventory->grid().ownerCell( mSelected ) != kNoCell ) {
		openMenu( inventory->grid(), equipped, injured );
	}
	return nothing;
}

void InventoryCursor::openMenu( const ItemGrid& grid, ItemId equipped, bool injured ) {
	ItemId item = grid.stackAt( grid.ownerCell( mSelected ) ).item;
	mMenuCount = 0;
	// 銃で、まだ手に持っていなければ Equip を一番上に（拾った銃をすぐ持てるように）。
	if ( weaponForItem( item ) != WEAPON_NONE && item != equipped ) {
		mMenuItems[ mMenuCount++ ] = MENU_EQUIP;
	}
	// 体力を戻す物は、減っているときだけ「使う」を一番上に（満タンでは出さない。R3_Plan.txt Q11）。
	if ( itemSpec( item ).kind == ITEM_KIND_MEDICAL && injured ) {
		mMenuItems[ mMenuCount++ ] = MENU_USE;
	}
	mMenuItems[ mMenuCount++ ] = MENU_DROP;
	mMenuItems[ mMenuCount++ ] = MENU_MOVE;
	mMenuItems[ mMenuCount++ ] = MENU_CANCEL;
	mMenuChoice = 0;
	mMode = Mode::Menu;
}

void InventoryCursor::selectMenu( int index ) {
	if ( index >= 0 && index < mMenuCount ) {
		mMenuChoice = index;
	}
}

void InventoryCursor::menuUp() {
	mMenuChoice = std::max( mMenuChoice - 1, 0 );
}

void InventoryCursor::menuDown() {
	mMenuChoice = std::min( mMenuChoice + 1, mMenuCount - 1 );
}

void InventoryCursor::closeMenu() {
	mMode = Mode::Browse;
}

InventoryCursor::Request InventoryCursor::activateMenu( const ItemGrid& grid ) {
	Request request = { Request::NONE, grid.ownerCell( mSelected ) };
	mMode = Mode::Browse;
	switch ( mMenuItems[ mMenuChoice ] ) {
	case MENU_EQUIP:
		request.kind = Request::EQUIP;
		break;
	case MENU_USE:
		request.kind = Request::USE;
		break;
	case MENU_DROP:
		request.kind = Request::DROP;
		break;
	case MENU_MOVE:
		mMovingFrom = request.cell;
		mMode = Mode::Moving;
		break;
	default:
		break; // Cancel
	}
	return request;
}

void InventoryCursor::place( Inventory* inventory ) {
	const ItemGrid& grid = inventory->grid();
	ItemId item = grid.stackAt( mMovingFrom ).item;
	// 元の場所を選んだら取りやめ（動かさない）。
	if ( grid.placementCell( item, mSelected ) == mMovingFrom ) {
		back();
		return;
	}
	if ( !inventory->canMove( mMovingFrom, mSelected ) ) {
		return; // 入らない所。赤い枠のまま、別の所を選んでもらう
	}
	inventory->move( mMovingFrom, mSelected );
	back();
}

InventoryCursor::Mode InventoryCursor::mode() const {
	return mMode;
}

bool InventoryCursor::isPlacing() const {
	return mMode == Mode::Placing;
}

int InventoryCursor::selected() const {
	return mSelected;
}

int InventoryCursor::movingFrom() const {
	return mMovingFrom;
}

int InventoryCursor::menuCount() const {
	return mMenuCount;
}

InventoryCursor::MenuItem InventoryCursor::menuItem( int index ) const {
	ASSERT( index >= 0 && index < mMenuCount && "InventoryCursor::menuItem : bad index." );
	return mMenuItems[ index ];
}

int InventoryCursor::menuChoice() const {
	return mMenuChoice;
}
