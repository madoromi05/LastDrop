#include "GameSrc/Scene/Game/InventoryScreen.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <algorithm>
#include <string>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
	const int kNone = -1;

	// マス 1 つの大きさと間（画素）。物の絵（1 マスの物は正方形）が潰れない正方形。
	// 座標の高さ 480 に、見出し・6 行・下の欄まで入る大きさ（強化で 7 行以上にするときは並べ方を変える。ItemIcon_Plan.txt 5 章）。
	const float kCellWidth = 64.f;
	const float kCellHeight = 64.f;
	const float kCellGap = 4.f;
	const float kIconInset = 3.f;      // マスの縁から絵まで（選んだ枠の太さより広く、枠が絵に掛からないように）
	const float kBothSides = 2.f;      // 左右（上下）の両側の余白の分
	// 数（2 桁）を置く右下の板。板の真ん中に数を置く。
	const float kAmountPlateWidth = 26.f;
	const float kAmountPlateHeight = 20.f; // 文字（UI_TEXT_NORMAL の 20）と同じ高さ
	const float kAmountInset = 3.f;    // マスの右下の角から板まで
	const float kHeadingGap = 34.f;    // 見出しの真ん中から、マス目の上端まで
	const float kFooterGap = 22.f;     // マス目の下端から、選んだ物の名前の真ん中まで
	const float kFooterLineHeight = 24.f;
	// 選んだ物のメニュー（マス目の右に、選んだ物の行から縦に並べる）。
	const float kMenuGap = 12.f;
	const float kMenuWidth = 100.f;
	const float kMenuHeight = 28.f;
	const float kMenuSpacing = 4.f;
	const float kFrameThickness = 2.f;

	// メニューの項目（全部）。物ごとに、使える物だけを上からこの順に並べる（openMenu）。
	enum MenuItem{
		MENU_EQUIP, // 銃を手に持つ（手に持っていない銃だけ）
		MENU_DROP,
		MENU_MOVE,
		MENU_CANCEL,

		MENU_ITEM_NUMBER,
	};
	const char* const kMenuLabels[ MENU_ITEM_NUMBER ] = { "Equip", "Drop", "Move", "Cancel" };
	// 手に持っている銃の印（マスの左上）。
	const char* kEquippedMark = "E";
	const float kMarkPlateSize = 20.f; // 文字（UI_TEXT_NORMAL の 20）と同じ高さの正方形

	const char* kHeading = "BAG";
	const char* kEmptyName = "Empty";
	const char* kBrowseHint = "[Enter] Select   [Tab] Close";
	const char* kMenuHint = "[Enter] Decide   [Esc] Back";
	const char* kMovingHint = "[Enter] Place   [Esc] Cancel";

	// 止めた画を暗くする板（一時停止と同じ暗さ）。
	const unsigned kDimColor = VertexColor::pack( 0.f, 0.f, 0.f, 0.6f );
	const unsigned kEmptyCellColor = VertexColor::pack( 0.08f, 0.08f, 0.08f, 0.85f );
	// 物のあるマスの背景（どの物も同じ。物の見分けは絵で付ける）。空のマスより少し明るくして、物のある所が分かるように。
	const float kItemCellGray = 0.2f;
	const float kItemAlpha = 0.9f;
	const float kOpaque = 1.f;        // 絵はそのままの色・濃さで貼る（白を掛ける）
	const float kLiftedAlpha = 0.35f; // 持ち上げている物の元の場所（薄くして、どれを動かしているか分かるように）
	const unsigned kAmountPlateColor = VertexColor::pack( 0.f, 0.f, 0.f, 0.65f );
	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );
	const unsigned kHintColor = VertexColor::pack( 0.65f, 0.65f, 0.65f );
	const unsigned kHeadingColor = VertexColor::pack( 0.85f, 0.65f, 0.15f ); // ボタンを選んだ色と同じ黄
	const unsigned kSelectColor = VertexColor::pack( 0.95f, 0.85f, 0.3f );
	const unsigned kPlaceColor = VertexColor::pack( 0.35f, 0.9f, 0.35f );   // 置ける所
	const unsigned kBlockedColor = VertexColor::pack( 0.9f, 0.2f, 0.15f );  // 入らない所
	const unsigned kMenuPlateColor = VertexColor::pack( 0.15f, 0.15f, 0.15f, 0.9f );
	const unsigned kMenuSelectedColor = VertexColor::pack( 0.85f, 0.65f, 0.15f, 0.95f );
	const unsigned kMenuSelectedTextColor = VertexColor::pack( 0.05f, 0.05f, 0.05f );

	unsigned itemColor( ItemId id, float alpha ) {
		const ItemSpec& spec = itemSpec( id );
		return VertexColor::pack( spec.red, spec.green, spec.blue, alpha );
	}

	// area の内側 kIconInset に、絵の縦横の比を保って収まる一番大きい長方形（真ん中に置く）。
	// 2 マスの物の範囲（132×64）は絵（2 : 1）とわずかに比が違うので、引き伸ばさずに収める。
	UiRect fitIcon( const UiRect& area, const Graphics::Texture& texture ) {
		float maxWidth = area.width - kIconInset * kBothSides;
		float maxHeight = area.height - kIconInset * kBothSides;
		float aspect = static_cast< float >( texture.originalWidth() ) / static_cast< float >( texture.originalHeight() );
		UiRect rect;
		rect.width = std::min( maxWidth, maxHeight * aspect );
		rect.height = rect.width / aspect;
		rect.left = area.left + ( area.width - rect.width ) * kHalf;
		rect.top = area.top + ( area.height - rect.height ) * kHalf;
		return rect;
	}

	float gridWidth( const ItemGrid& grid ) {
		return grid.columns() * kCellWidth + ( grid.columns() - 1 ) * kCellGap;
	}

	float gridHeight( const ItemGrid& grid ) {
		return grid.rows() * kCellHeight + ( grid.rows() - 1 ) * kCellGap;
	}

	// マス目の左上（画素。画面の真ん中に置く）。
	Vector2 gridOrigin( const ItemGrid& grid ) {
		float left = UiLayout::anchorX( UI_ANCHOR_CENTER ) - gridWidth( grid ) * kHalf;
		float top = Framework::instance().height() * kHalf - gridHeight( grid ) * kHalf;
		return Vector2( left, top );
	}
}

InventoryScreen::InventoryScreen( UiRenderer* renderer ) :
	mRenderer( renderer ),
	mMode( Mode::Browse ),
	mSelected( 0 ),
	mMenuCount( 0 ),
	mMenuChoice( 0 ),
	mMovingFrom( kNone ),
	mPreviousCursor( 0.f, 0.f ) {
	static_assert( MENU_ITEM_NUMBER == kMenuCapacity, "kMenuCapacity must hold every MenuItem" );
	ASSERT( mRenderer && "InventoryScreen : renderer is null." );
}

void InventoryScreen::open() {
	mMode = Mode::Browse;
	mSelected = 0;
	mMenuCount = 0;
	mMenuChoice = 0;
	mMovingFrom = kNone;
}

bool InventoryScreen::back() {
	if ( mMode == Mode::Browse ) {
		return false;
	}
	mMode = Mode::Browse;
	mMovingFrom = kNone;
	return true;
}

InventoryScreen::Request InventoryScreen::handleInput( Inventory* inventory, ItemId equipped ) {
	const Request nothing = { Request::NONE, kNone };
	ASSERT( inventory && "InventoryScreen::handleInput : inventory is null." );
	UiInput input = UiInput::read( mPreviousCursor );
	mPreviousCursor = input.cursor;
	const ItemGrid& grid = inventory->grid();
	forgetStale( grid );

	if ( mMode == Mode::Menu ) {
		int pointed = menuAt( grid, input.cursor );
		if ( input.cursorMoved && pointed != kNone ) {
			mMenuChoice = pointed;
		}
		if ( input.upPressed ) {
			mMenuChoice = std::max( mMenuChoice - 1, 0 );
		}
		if ( input.downPressed ) {
			mMenuChoice = std::min( mMenuChoice + 1, mMenuCount - 1 );
		}
		if ( input.clicked ) {
			if ( pointed == kNone ) {
				mMode = Mode::Browse; // メニューの外を押したら閉じる
				return nothing;
			}
			mMenuChoice = pointed;
			return activateMenu( grid );
		}
		if ( input.decidePressed ) {
			return activateMenu( grid );
		}
		return nothing;
	}

	// Browse と Moving は、どちらもマスを選ぶ（Moving は置く所を選ぶ）。
	int pointed = cellAt( grid, input.cursor );
	if ( input.cursorMoved && pointed != kNone ) {
		mSelected = pointed;
	}
	moveSelection( input.upPressed, input.downPressed, input.leftPressed, input.rightPressed, grid );
	bool decided = input.decidePressed;
	if ( input.clicked && pointed != kNone ) {
		mSelected = pointed;
		decided = true;
	}
	if ( !decided ) {
		return nothing;
	}
	if ( mMode == Mode::Moving ) {
		place( inventory );
	} else if ( grid.ownerCell( mSelected ) != kNone ) {
		openMenu( grid, equipped );
	}
	return nothing;
}

void InventoryScreen::forgetStale( const ItemGrid& grid ) {
	// F6 で行を減らした・物を足したときに、消えたマスや、もう無い物を指したままにしない。
	mSelected = std::min( mSelected, grid.cellCount() - 1 );
	if ( mMode == Mode::Moving && ( mMovingFrom >= grid.cellCount() || grid.ownerCell( mMovingFrom ) != mMovingFrom ) ) {
		back();
	}
	if ( mMode == Mode::Menu && grid.ownerCell( mSelected ) == kNone ) {
		back();
	}
}

void InventoryScreen::moveSelection( bool up, bool down, bool left, bool right, const ItemGrid& grid ) {
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

void InventoryScreen::openMenu( const ItemGrid& grid, ItemId equipped ) {
	ItemId item = grid.stackAt( grid.ownerCell( mSelected ) ).item;
	mMenuCount = 0;
	// 銃で、まだ手に持っていなければ Equip を一番上に（拾った銃をすぐ持てるように）。
	if ( weaponForItem( item ) != WEAPON_NONE && item != equipped ) {
		mMenuItems[ mMenuCount++ ] = MENU_EQUIP;
	}
	mMenuItems[ mMenuCount++ ] = MENU_DROP;
	mMenuItems[ mMenuCount++ ] = MENU_MOVE;
	mMenuItems[ mMenuCount++ ] = MENU_CANCEL;
	mMenuChoice = 0;
	mMode = Mode::Menu;
}

InventoryScreen::Request InventoryScreen::activateMenu( const ItemGrid& grid ) {
	Request request = { Request::NONE, grid.ownerCell( mSelected ) };
	mMode = Mode::Browse;
	switch ( mMenuItems[ mMenuChoice ] ) {
	case MENU_EQUIP:
		request.kind = Request::EQUIP;
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

void InventoryScreen::place( Inventory* inventory ) {
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

UiRect InventoryScreen::cellRect( const ItemGrid& grid, int cell ) const {
	return spanRect( grid, cell, 1 );
}

UiRect InventoryScreen::spanRect( const ItemGrid& grid, int owner, int size ) const {
	Vector2 origin = gridOrigin( grid );
	UiRect rect;
	rect.left = origin.x + ( owner % grid.columns() ) * ( kCellWidth + kCellGap );
	rect.top = origin.y + ( owner / grid.columns() ) * ( kCellHeight + kCellGap );
	rect.width = size * kCellWidth + ( size - 1 ) * kCellGap;
	rect.height = kCellHeight;
	return rect;
}

UiRect InventoryScreen::menuRect( const ItemGrid& grid, int index ) const {
	Vector2 origin = gridOrigin( grid );
	UiRect rect;
	rect.left = origin.x + gridWidth( grid ) + kMenuGap;
	rect.top = cellRect( grid, mSelected ).top + index * ( kMenuHeight + kMenuSpacing );
	rect.width = kMenuWidth;
	rect.height = kMenuHeight;
	return rect;
}

int InventoryScreen::cellAt( const ItemGrid& grid, const Vector2& point ) const {
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( cellRect( grid, i ).contains( point ) ) {
			return i;
		}
	}
	return kNone;
}

int InventoryScreen::menuAt( const ItemGrid& grid, const Vector2& point ) const {
	for ( int i = 0; i < mMenuCount; ++i ) {
		if ( menuRect( grid, i ).contains( point ) ) {
			return i;
		}
	}
	return kNone;
}

void InventoryScreen::draw( const Inventory& inventory, ItemId equipped ) {
	const ItemGrid& grid = inventory.grid();
	forgetStale( grid );
	// 板を先に積む（UiRenderer は積んだ順に長方形を描くので、マスより下になる）。
	UiRect screen;
	screen.left = 0.f;
	screen.top = 0.f;
	screen.width = static_cast< float >( Framework::instance().width() );
	screen.height = static_cast< float >( Framework::instance().height() );
	mRenderer->addRect( screen, kDimColor );

	Vector2 heading( UiLayout::anchorX( UI_ANCHOR_CENTER ), gridOrigin( grid ).y - kHeadingGap );
	mRenderer->addText( kHeading, heading, UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor );
	drawCells( grid );
	drawEquippedMark( grid, equipped );
	drawSelection( inventory );
	if ( mMode == Mode::Menu ) {
		drawMenu( grid );
	}
	drawFooter( grid );
	mRenderer->draw();
}

void InventoryScreen::drawCells( const ItemGrid& grid ) {
	// ① 背景（長方形だけ）。物のマスは同じ灰。絵が読めなかった物だけ、物の色で塗る（名前は選ぶと下の欄に出る）。
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) == kNone ) {
			mRenderer->addRect( cellRect( grid, i ), kEmptyCellColor );
		}
	}
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) != i ) {
			continue; // 空のマスか、2 マスの物の右のマス
		}
		ItemId item = grid.stackAt( i ).item;
		float alpha = ( mMode == Mode::Moving && i == mMovingFrom ) ? kLiftedAlpha : kItemAlpha;
		unsigned color = mIcons.icon( item ) ? VertexColor::pack( kItemCellGray, kItemCellGray, kItemCellGray, alpha ) : itemColor( item, alpha );
		mRenderer->addRect( spanRect( grid, i, itemSpec( item ).cellSize ), color );
	}
	// ② 物の絵（まとめて積む）。持ち上げている物の元の場所は、背景と同じく薄くする。
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) != i ) {
			continue;
		}
		ItemId item = grid.stackAt( i ).item;
		const Graphics::Texture* icon = mIcons.icon( item );
		if ( !icon ) {
			continue;
		}
		float alpha = ( mMode == Mode::Moving && i == mMovingFrom ) ? kLiftedAlpha : kOpaque;
		UiRect area = spanRect( grid, i, itemSpec( item ).cellSize );
		mRenderer->addImage( fitIcon( area, *icon ), *icon, VertexColor::pack( kOpaque, kOpaque, kOpaque, alpha ) );
	}
	// ③ 数（絵の上に重ねる）。
	drawAmounts( grid );
}

void InventoryScreen::drawAmounts( const ItemGrid& grid ) {
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) != i ) {
			continue;
		}
		const ItemStack& stack = grid.stackAt( i );
		const ItemSpec& spec = itemSpec( stack.item );
		// 重ねられる物だけ数を出す（ガソリンのように 1 つずつの物は数を出さない）。
		if ( spec.stackSize <= 1 ) {
			continue;
		}
		UiRect area = spanRect( grid, i, spec.cellSize );
		UiRect plate;
		plate.width = kAmountPlateWidth;
		plate.height = kAmountPlateHeight;
		plate.left = area.right() - kAmountInset - plate.width;
		plate.top = area.bottom() - kAmountInset - plate.height;
		mRenderer->addRect( plate, kAmountPlateColor );
		// 板の真ん中に置く（2 桁まで。文字の幅は UiRenderer の外から測れないので、右揃えではなく板の中央揃えにする）。
		std::string amount = std::to_string( stack.amount );
		mRenderer->addText( amount.c_str(), plate.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	}
}

void InventoryScreen::drawEquippedMark( const ItemGrid& grid, ItemId equipped ) {
	if ( equipped == ITEM_NONE ) {
		return; // 素手
	}
	for ( int i = 0; i < grid.cellCount(); ++i ) {
		if ( grid.ownerCell( i ) != i || grid.stackAt( i ).item != equipped ) {
			continue;
		}
		UiRect area = cellRect( grid, i );
		UiRect plate;
		plate.left = area.left + kAmountInset;
		plate.top = area.top + kAmountInset;
		plate.width = kMarkPlateSize;
		plate.height = kMarkPlateSize;
		mRenderer->addRect( plate, kAmountPlateColor );
		mRenderer->addText( kEquippedMark, plate.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHeadingColor );
		return; // 銃は 1 種類 1 丁だけ
	}
}

void InventoryScreen::drawSelection( const Inventory& inventory ) {
	const ItemGrid& grid = inventory.grid();
	if ( mMode == Mode::Moving ) {
		// 置く所に、持ち上げている物の大きさの枠。入らない所は赤。
		ItemId item = grid.stackAt( mMovingFrom ).item;
		int target = grid.placementCell( item, mSelected );
		bool fits = ( target == mMovingFrom ) || inventory.canMove( mMovingFrom, mSelected );
		addFrame( spanRect( grid, target, itemSpec( item ).cellSize ), kFrameThickness, fits ? kPlaceColor : kBlockedColor );
		return;
	}
	// 物のマスなら、その物全体（2 マスの物は 1 行）に枠。
	int owner = grid.ownerCell( mSelected );
	if ( owner == kNone ) {
		addFrame( cellRect( grid, mSelected ), kFrameThickness, kSelectColor );
	} else {
		addFrame( spanRect( grid, owner, itemSpec( grid.stackAt( owner ).item ).cellSize ), kFrameThickness, kSelectColor );
	}
}

void InventoryScreen::drawMenu( const ItemGrid& grid ) {
	for ( int i = 0; i < mMenuCount; ++i ) {
		UiRect rect = menuRect( grid, i );
		bool selected = ( i == mMenuChoice );
		mRenderer->addRect( rect, selected ? kMenuSelectedColor : kMenuPlateColor );
		mRenderer->addText( kMenuLabels[ mMenuItems[ i ] ], rect.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, selected ? kMenuSelectedTextColor : kTextColor );
	}
}

void InventoryScreen::drawFooter( const ItemGrid& grid ) {
	Vector2 position( UiLayout::anchorX( UI_ANCHOR_CENTER ), gridOrigin( grid ).y + gridHeight( grid ) + kFooterGap );
	int owner = grid.ownerCell( mSelected );
	const char* name = ( owner == kNone ) ? kEmptyName : itemSpec( grid.stackAt( owner ).item ).name;
	if ( mMode == Mode::Moving ) {
		name = itemSpec( grid.stackAt( mMovingFrom ).item ).name; // 持ち上げている物
	}
	mRenderer->addText( name, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	const char* hint = kBrowseHint;
	if ( mMode == Mode::Menu ) {
		hint = kMenuHint;
	} else if ( mMode == Mode::Moving ) {
		hint = kMovingHint;
	}
	position.y += kFooterLineHeight;
	mRenderer->addText( hint, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor );
}

void InventoryScreen::addFrame( const UiRect& rect, float thickness, unsigned color ) {
	UiRect edge = rect;
	edge.height = thickness;
	mRenderer->addRect( edge, color ); // 上
	edge.top = rect.bottom() - thickness;
	mRenderer->addRect( edge, color ); // 下
	edge.top = rect.top;
	edge.height = rect.height;
	edge.width = thickness;
	mRenderer->addRect( edge, color ); // 左
	edge.left = rect.right() - thickness;
	mRenderer->addRect( edge, color ); // 右
}
