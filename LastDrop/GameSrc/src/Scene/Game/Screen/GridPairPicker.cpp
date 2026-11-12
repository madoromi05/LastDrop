#include "GameSrc/Scene/Game/Screen/GridPairPicker.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include <algorithm>

using namespace GameLib::Math;

namespace{
	const int kNone = -1;
	const float kFrameThickness = 2.f;
	const unsigned kSelectColor = VertexColor::pack( 0.95f, 0.85f, 0.3f );
	const unsigned kPlaceColor = VertexColor::pack( 0.35f, 0.9f, 0.35f );   // 置ける所
	const unsigned kBlockedColor = VertexColor::pack( 0.9f, 0.2f, 0.15f );  // 入らない所

	GridPairPicker::Side otherSide( GridPairPicker::Side side ) {
		return ( side == GridPairPicker::Side::Left ) ? GridPairPicker::Side::Right : GridPairPicker::Side::Left;
	}
}

GridPairPicker::GridPairPicker( UiRenderer* renderer, ItemIcons* icons, GridRule leftRule, GridRule rightRule ) :
	mLeftView( renderer, icons ),
	mRightView( renderer, icons ),
	mLeftRule( leftRule ),
	mRightRule( rightRule ),
	mFocus( Side::Left ),
	mSelected( 0 ),
	mMoving( false ),
	mMovingSide( Side::Left ),
	mMovingFrom( kNone ) {
}

void GridPairPicker::setCellAlpha( float alpha ) {
	mLeftView.setCellAlpha( alpha );
	mRightView.setCellAlpha( alpha );
}

void GridPairPicker::layout( const Vector2& leftOrigin, const Vector2& rightOrigin, float cellSize ) {
	mLeftView.setLayout( leftOrigin, cellSize );
	mRightView.setLayout( rightOrigin, cellSize );
}

const ItemGridView& GridPairPicker::view( Side side ) const {
	return ( side == Side::Left ) ? mLeftView : mRightView;
}

ItemGridView& GridPairPicker::view( Side side ) {
	return viewOf( side );
}

ItemGridView& GridPairPicker::viewOf( Side side ) {
	return ( side == Side::Left ) ? mLeftView : mRightView;
}

const ItemGrid& GridPairPicker::gridOf( const ItemGrid& left, const ItemGrid& right, Side side ) const {
	return ( side == Side::Left ) ? left : right;
}

ItemGrid* GridPairPicker::gridOf( ItemGrid* left, ItemGrid* right, Side side ) const {
	return ( side == Side::Left ) ? left : right;
}

void GridPairPicker::setRightRule( GridRule rule ) {
	mRightRule = rule;
}

GridRule GridPairPicker::ruleOf( Side side ) const {
	return ( side == Side::Left ) ? mLeftRule : mRightRule;
}

void GridPairPicker::reset( Side side, int cell ) {
	select( side, cell );
	mMoving = false;
}

void GridPairPicker::select( Side side, int cell ) {
	mFocus = side;
	mSelected = cell;
}

bool GridPairPicker::back() {
	if ( !mMoving ) {
		return false;
	}
	mMoving = false;
	return true;
}

bool GridPairPicker::isMoving() const {
	return mMoving;
}

GridPairPicker::Side GridPairPicker::focus() const {
	return mFocus;
}

int GridPairPicker::selected() const {
	return mSelected;
}

void GridPairPicker::forgetStale( const ItemGrid& left, const ItemGrid& right ) {
	mSelected = std::min( mSelected, gridOf( left, right, mFocus ).cellCount() - 1 );
	if ( mMoving ) {
		const ItemGrid& source = gridOf( left, right, mMovingSide );
		if ( mMovingFrom >= source.cellCount() || source.ownerCell( mMovingFrom ) != mMovingFrom ) {
			mMoving = false;
		}
	}
}

bool GridPairPicker::point( const ItemGrid& left, const ItemGrid& right, const Vector2& cursor ) {
	int leftCell = mLeftView.cellAt( left, cursor );
	if ( leftCell != kNone ) {
		select( Side::Left, leftCell );
		return true;
	}
	int rightCell = mRightView.cellAt( right, cursor );
	if ( rightCell != kNone ) {
		select( Side::Right, rightCell );
		return true;
	}
	return false;
}

GridPairPicker::Exit GridPairPicker::moveSelection( bool up, bool down, bool left, bool right, const ItemGrid& leftGrid, const ItemGrid& rightGrid ) {
	const ItemGrid& current = gridOf( leftGrid, rightGrid, mFocus );
	int row = mSelected / current.columns();
	int column = mSelected % current.columns();
	if ( up ) {
		row = std::max( row - 1, 0 );
	}
	if ( down ) {
		if ( mFocus == Side::Right && row == current.rows() - 1 ) {
			return Exit::BelowRight;
		}
		row = std::min( row + 1, current.rows() - 1 );
	}
	if ( left && column == 0 && mFocus == Side::Right ) {
		// 右のマス目の左端から、左のマス目の右端へ（同じ行に収める）
		select( Side::Left, std::min( row, leftGrid.rows() - 1 ) * leftGrid.columns() + leftGrid.columns() - 1 );
		return Exit::None;
	}
	if ( right && column == current.columns() - 1 && mFocus == Side::Left ) {
		// 左のマス目の右端から、右のマス目の左端へ
		select( Side::Right, std::min( row, rightGrid.rows() - 1 ) * rightGrid.columns() );
		return Exit::None;
	}
	if ( left ) {
		column = std::max( column - 1, 0 );
	}
	if ( right ) {
		column = std::min( column + 1, current.columns() - 1 );
	}
	mSelected = row * current.columns() + column;
	return Exit::None;
}

bool GridPairPicker::isLiftedSpot( const ItemGrid& left, const ItemGrid& right ) const {
	const ItemGrid& source = gridOf( left, right, mMovingSide );
	return mMovingSide == mFocus && source.placementCell( source.stackAt( mMovingFrom ).item, mSelected ) == mMovingFrom;
}

bool GridPairPicker::canPlace( const ItemGrid& left, const ItemGrid& right ) const {
	if ( isLiftedSpot( left, right ) ) {
		return true; // 取りやめ
	}
	return ItemTransfer::canMove( gridOf( left, right, mMovingSide ), ruleOf( mMovingSide ), mMovingFrom,
		gridOf( left, right, mFocus ), ruleOf( mFocus ), mSelected );
}

void GridPairPicker::decide( ItemGrid* left, ItemGrid* right ) {
	ASSERT( left && right && "GridPairPicker::decide : grid is null." );
	ItemGrid* current = gridOf( left, right, mFocus );
	if ( !mMoving ) {
		int owner = current->ownerCell( mSelected );
		if ( owner != kNone ) {
			mMoving = true;
			mMovingSide = mFocus;
			mMovingFrom = owner;
		}
		return;
	}
	if ( isLiftedSpot( *left, *right ) ) {
		mMoving = false; // 元の場所を選んだら取りやめ（動かさない）
		return;
	}
	if ( !canPlace( *left, *right ) ) {
		return; // 入らない所。赤い枠のまま、別の所を選んでもらう
	}
	ItemTransfer::move( gridOf( left, right, mMovingSide ), mMovingFrom, current, mSelected );
	mMoving = false;
}

bool GridPairPicker::quickMove( ItemGrid* left, ItemGrid* right ) {
	ASSERT( left && right && "GridPairPicker::quickMove : grid is null." );
	if ( mMoving ) {
		return false;
	}
	Side target = otherSide( mFocus );
	return ItemTransfer::quickMove( gridOf( left, right, mFocus ), ruleOf( mFocus ), mSelected,
		gridOf( left, right, target ), ruleOf( target ) );
}

const ItemStack* GridPairPicker::focusedStack( const ItemGrid& left, const ItemGrid& right ) const {
	if ( mMoving ) {
		return &gridOf( left, right, mMovingSide ).stackAt( mMovingFrom );
	}
	const ItemGrid& current = gridOf( left, right, mFocus );
	int owner = current.ownerCell( mSelected );
	return ( owner == kNone ) ? nullptr : &current.stackAt( owner );
}

void GridPairPicker::draw( const ItemGrid& left, const ItemGrid& right, bool showSelection ) {
	mLeftView.drawCells( left, ( mMoving && mMovingSide == Side::Left ) ? mMovingFrom : kNone );
	mRightView.drawCells( right, ( mMoving && mMovingSide == Side::Right ) ? mMovingFrom : kNone );
	if ( !showSelection ) {
		return;
	}
	const ItemGrid& current = gridOf( left, right, mFocus );
	ItemGridView& view = viewOf( mFocus );
	if ( mMoving ) {
		// 置く所に、持ち上げている物の大きさの枠。入らない所は赤
		ItemId item = gridOf( left, right, mMovingSide ).stackAt( mMovingFrom ).item;
		int target = current.placementCell( item, mSelected );
		view.addFrame( view.spanRect( current, target, itemSpec( item ).cellSize ), kFrameThickness,
			canPlace( left, right ) ? kPlaceColor : kBlockedColor );
		return;
	}
	int owner = current.ownerCell( mSelected );
	if ( owner == kNone ) {
		view.addFrame( view.cellRect( current, mSelected ), kFrameThickness, kSelectColor );
	} else {
		view.addFrame( view.spanRect( current, owner, itemSpec( current.stackAt( owner ).item ).cellSize ), kFrameThickness, kSelectColor );
	}
}
