#include "GameSrc/Scene/Game/InventoryScreen.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/Core/Pad.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <algorithm>
#include <cstdio>
#include <string>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
	const int kNone = -1;

	// マス 1 つの一辺（画素）。物の絵（1 マスの物は正方形）が潰れない正方形。
	// 座標の高さ 480 に、見出し・6 行・下の欄まで入る大きさ（強化で 7 行以上にするときは並べ方を変える。ItemIcon_Plan.txt 5 章）。
	const float kCellSize = 64.f;
	const float kHeadingGap = 34.f;    // 見出しの真ん中から、マス目の上端まで
	const float kExitsGap = 12.f;      // 開いている脱出点の行の真ん中から、マス目の上端まで（見出しとマス目の間）
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

	const char* kHeading = "BAG";
	const char* kEmptyName = "Empty";
	const char* kBrowseHint = "[Enter] Select   [Tab] Close";
	const char* kMenuHint = "[Enter] Decide   [Esc] Back";
	const char* kMovingHint = "[Enter] Place   [Esc] Cancel";
	const char* kPlacingHint = "[Enter] Put   [F] Auto   [Esc] Cancel";
	const char* kPickupFormat = "Pick up: %s";           // 1 つずつの物（ガソリン・銃）
	const char* kPickupAmountFormat = "Pick up: %s x%d"; // 重ねられる物（弾）は数も
	const int kTextSize = 64;

	// 後ろの町を透かして見せる濃さ（InventoryPickup_Plan.txt Q6。ユーザー指定の 0.5）。暗くする板とマスの背景に使い、物の絵と文字は濃いまま。
	// 一時停止（0.6）より薄い。
	const float kPanelAlpha = 0.5f;
	const unsigned kDimColor = VertexColor::pack( 0.f, 0.f, 0.f, kPanelAlpha );
	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );
	const unsigned kHintColor = VertexColor::pack( 0.65f, 0.65f, 0.65f );
	const unsigned kHeadingColor = VertexColor::pack( 0.85f, 0.65f, 0.15f ); // ボタンを選んだ色と同じ黄
	const unsigned kSelectColor = VertexColor::pack( 0.95f, 0.85f, 0.3f );
	const unsigned kPlaceColor = VertexColor::pack( 0.35f, 0.9f, 0.35f );   // 置ける所
	const unsigned kBlockedColor = VertexColor::pack( 0.9f, 0.2f, 0.15f );  // 入らない所
	const unsigned kMenuPlateColor = VertexColor::pack( 0.15f, 0.15f, 0.15f, 0.9f );
	const unsigned kMenuSelectedColor = VertexColor::pack( 0.85f, 0.65f, 0.15f, 0.95f );
	const unsigned kMenuSelectedTextColor = VertexColor::pack( 0.05f, 0.05f, 0.05f );

	float gridWidth( const ItemGrid& grid ) {
		return ItemGridView::width( grid, kCellSize );
	}

	float gridHeight( const ItemGrid& grid ) {
		return ItemGridView::height( grid, kCellSize );
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
	mView( renderer, &mIcons ),
	mMode( Mode::Browse ),
	mSelected( 0 ),
	mMenuCount( 0 ),
	mMenuChoice( 0 ),
	mMovingFrom( kNone ),
	mPreviousCursor( 0.f, 0.f ) {
	static_assert( MENU_ITEM_NUMBER == kMenuCapacity, "kMenuCapacity must hold every MenuItem" );
	ASSERT( mRenderer && "InventoryScreen : renderer is null." );
	mView.setCellAlpha( kPanelAlpha );
}

void InventoryScreen::openForPlacing( const Inventory& inventory, const ItemStack& incoming ) {
	mMode = Mode::Placing;
	mSelected = 0;
	for ( int cell = 0; cell < inventory.grid().cellCount(); ++cell ) {
		if ( inventory.countPlaceableInto( cell, incoming ) > 0 ) {
			mSelected = cell; // 入る最初のマス（決定を 1 回押すだけで入れられるように）
			break;
		}
	}
}

bool InventoryScreen::isPlacing() const {
	return mMode == Mode::Placing;
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

InventoryScreen::Request InventoryScreen::handleInput( Inventory* inventory, ItemId equipped, const ItemStack* incoming ) {
	const Request nothing = { Request::NONE, kNone };
	ASSERT( inventory && "InventoryScreen::handleInput : inventory is null." );
	UiInput input = UiInput::read( mPreviousCursor );
	mPreviousCursor = input.cursor;
	const ItemGrid& grid = inventory->grid();
	forgetStale( grid );
	layout( grid );
	if ( mMode == Mode::Placing && !incoming ) {
		mMode = Mode::Browse; // 拾う物が無くなった（全部入れた）
	}

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

	// Browse・Moving・Placing は、どれもマスを選ぶ（Moving・Placing は置く所を選ぶ）。
	int pointed = mView.cellAt( grid, input.cursor );
	if ( input.cursorMoved && pointed != kNone ) {
		mSelected = pointed;
	}
	moveSelection( input.upPressed, input.downPressed, input.leftPressed, input.rightPressed, grid );
	if ( mMode == Mode::Placing && InputQuickPlace() ) {
		return { Request::QUICK_PLACE, kNone };
	}
	bool decided = input.decidePressed;
	if ( input.clicked && pointed != kNone ) {
		mSelected = pointed;
		decided = true;
	}
	if ( !decided ) {
		return nothing;
	}
	if ( mMode == Mode::Placing ) {
		// 入らない所なら何もしない（赤い枠のまま、別の所を選んでもらう）。入れるのは本編（拾う物から減らすのも本編）。
		return ( inventory->countPlaceableInto( mSelected, *incoming ) > 0 ) ? Request{ Request::PLACE, mSelected } : nothing;
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

void InventoryScreen::layout( const ItemGrid& grid ) {
	mView.setLayout( gridOrigin( grid ), kCellSize );
}

UiRect InventoryScreen::menuRect( const ItemGrid& grid, int index ) const {
	Vector2 origin = gridOrigin( grid );
	UiRect rect;
	rect.left = origin.x + gridWidth( grid ) + kMenuGap;
	rect.top = mView.cellRect( grid, mSelected ).top + index * ( kMenuHeight + kMenuSpacing );
	rect.width = kMenuWidth;
	rect.height = kMenuHeight;
	return rect;
}

int InventoryScreen::menuAt( const ItemGrid& grid, const Vector2& point ) const {
	for ( int i = 0; i < mMenuCount; ++i ) {
		if ( menuRect( grid, i ).contains( point ) ) {
			return i;
		}
	}
	return kNone;
}

void InventoryScreen::draw( const Inventory& inventory, ItemId equipped, const char* exits, const ItemStack* incoming ) {
	const ItemGrid& grid = inventory.grid();
	forgetStale( grid );
	layout( grid );
	// 板を先に積む（UiRenderer は積んだ順に長方形を描くので、マスより下になる）。
	UiRect screen;
	screen.left = 0.f;
	screen.top = 0.f;
	screen.width = static_cast< float >( Framework::instance().width() );
	screen.height = static_cast< float >( Framework::instance().height() );
	mRenderer->addRect( screen, kDimColor );

	Vector2 heading( UiLayout::anchorX( UI_ANCHOR_CENTER ), gridOrigin( grid ).y - kHeadingGap );
	mRenderer->addText( kHeading, heading, UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor );
	if ( exits ) {
		Vector2 exitsLine( heading.x, gridOrigin( grid ).y - kExitsGap );
		mRenderer->addText( exits, exitsLine, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor );
	}
	mView.drawCells( grid, ( mMode == Mode::Moving ) ? mMovingFrom : kNone );
	mView.drawMark( grid, equipped, kEquippedMark );
	drawSelection( inventory, incoming );
	if ( mMode == Mode::Menu ) {
		drawMenu( grid );
	}
	drawFooter( grid, incoming );
	mRenderer->draw();
}

void InventoryScreen::drawSelection( const Inventory& inventory, const ItemStack* incoming ) {
	const ItemGrid& grid = inventory.grid();
	if ( mMode == Mode::Placing && incoming ) {
		// 入れようとしている所に、拾った物を薄く重ね、その大きさの枠。入らない所は赤（入るのは空き・同じ物に重ねるだけ）。
		int target = grid.placementCell( incoming->item, mSelected );
		bool fits = inventory.countPlaceableInto( mSelected, *incoming ) > 0;
		mView.drawGhost( grid, target, *incoming );
		mView.addFrame( mView.spanRect( grid, target, itemSpec( incoming->item ).cellSize ), kFrameThickness, fits ? kPlaceColor : kBlockedColor );
		return;
	}
	if ( mMode == Mode::Moving ) {
		// 置く所に、持ち上げている物の大きさの枠。入らない所は赤。
		ItemId item = grid.stackAt( mMovingFrom ).item;
		int target = grid.placementCell( item, mSelected );
		bool fits = ( target == mMovingFrom ) || inventory.canMove( mMovingFrom, mSelected );
		mView.addFrame( mView.spanRect( grid, target, itemSpec( item ).cellSize ), kFrameThickness, fits ? kPlaceColor : kBlockedColor );
		return;
	}
	// 物のマスなら、その物全体（2 マスの物は 1 行）に枠。
	int owner = grid.ownerCell( mSelected );
	if ( owner == kNone ) {
		mView.addFrame( mView.cellRect( grid, mSelected ), kFrameThickness, kSelectColor );
	} else {
		mView.addFrame( mView.spanRect( grid, owner, itemSpec( grid.stackAt( owner ).item ).cellSize ), kFrameThickness, kSelectColor );
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

void InventoryScreen::drawFooter( const ItemGrid& grid, const ItemStack* incoming ) {
	Vector2 position( UiLayout::anchorX( UI_ANCHOR_CENTER ), gridOrigin( grid ).y + gridHeight( grid ) + kFooterGap );
	int owner = grid.ownerCell( mSelected );
	const char* name = ( owner == kNone ) ? kEmptyName : itemSpec( grid.stackAt( owner ).item ).name;
	char text[ kTextSize ];
	if ( mMode == Mode::Moving ) {
		name = itemSpec( grid.stackAt( mMovingFrom ).item ).name; // 持ち上げている物
	} else if ( mMode == Mode::Placing && incoming ) {
		// 拾って持っている物（重ねられる物は、まだ入れていない数も）。
		const ItemSpec& spec = itemSpec( incoming->item );
		if ( spec.stackSize > 1 ) {
			std::snprintf( text, sizeof( text ), kPickupAmountFormat, spec.name, incoming->amount );
		} else {
			std::snprintf( text, sizeof( text ), kPickupFormat, spec.name );
		}
		name = text;
	}
	mRenderer->addText( name, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	const char* hint = kBrowseHint;
	if ( mMode == Mode::Menu ) {
		hint = kMenuHint;
	} else if ( mMode == Mode::Placing ) {
		hint = kPlacingHint;
	} else if ( mMode == Mode::Moving ) {
		hint = kMovingHint;
	}
	position.y += kFooterLineHeight;
	mRenderer->addText( hint, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor );
}
