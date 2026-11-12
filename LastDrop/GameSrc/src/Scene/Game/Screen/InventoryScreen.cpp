#include "GameSrc/Scene/Game/Screen/InventoryScreen.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/Player/PlayerHealth.h"

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

	// マス 1 つの一辺（画素）。物の絵（1 マスの物は正方形）が潰れない正方形
	// 座標の高さ 480 に、見出し・6 行・下の欄まで入る大きさ（強化で 7 行以上にするときは並べ方を変える）
	const float kCellSize = 64.f;
	const float kHeadingGap = 34.f;    // 見出しの真ん中から、マス目の上端まで
	const float kExitsGap = 12.f;      // 開いている脱出点の行の真ん中から、マス目の上端まで（見出しとマス目の間）
	const float kFooterGap = 22.f;     // マス目の下端から、選んだ物の名前の真ん中まで
	const float kFooterLineHeight = 24.f;
	// 選んだ物のメニュー（マス目の右に、選んだ物の行から縦に並べる）
	const float kMenuGap = 12.f;
	const float kMenuWidth = 100.f;
	const float kMenuHeight = 28.f;
	const float kMenuSpacing = 4.f;
	const float kFrameThickness = 2.f;
	// 左下の心電図パネルの位置と大きさ
	const float kEcgMargin = 14.f;
	const float kEcgWidth = 210.f;
	const float kEcgHeight = 56.f;

	const char* const kMenuLabels[] = { "装備", "使う", "捨てる", "動かす", "やめる" };	// メニューの項目の文字（InventoryCursor::MenuItem の順）
	const char* kEquippedMark = "E";													// 手に持っている銃の印（マスの左上）

	const char* kHeading = "カバン";
	const char* kEmptyName = "空";
	const char* kBrowseHint = "[Enter] 選ぶ   [Tab] 閉じる";
	const char* kMenuHint = "[Enter] 決定   [Esc] 戻る";
	const char* kMovingHint = "[Enter] 置く   [Esc] やめる";
	const char* kPlacingHint = "[Enter] 置く   [F] 自動で置く   [Esc] やめる";
	const char* kPickupFormat = "拾う：%s";           // 1 つずつの物（ガソリン・銃）
	const char* kPickupAmountFormat = "拾う：%s ×%d"; // 重ねられる物（弾）は数も
	const int kTextSize = 64;

	// 後ろの町を透かして見せる濃さ。暗くする板とマスの背景に使い、物の絵と文字は濃いまま
	// 一時停止（0.6）より薄い
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

	// マス目の左上（画素。画面の真ん中に置く）
	Vector2 gridOrigin( const ItemGrid& grid ) {
		float left = UiLayout::anchorX( UI_ANCHOR_CENTER ) - gridWidth( grid ) * kHalf;
		float top = Framework::instance().height() * kHalf - gridHeight( grid ) * kHalf;
		return Vector2( left, top );
	}
}

InventoryScreen::InventoryScreen( UiRenderer* renderer ) :
	mRenderer( renderer ),
	mView( renderer, &mIcons ),
	mEcg( renderer ),
	mPreviousCursor( 0.f, 0.f ) {
	static_assert( sizeof( kMenuLabels ) / sizeof( kMenuLabels[ 0 ] ) == InventoryCursor::MENU_ITEM_NUMBER, "kMenuLabels must name every MenuItem" );
	ASSERT( mRenderer && "InventoryScreen : renderer is null." );
	mView.setCellAlpha( kPanelAlpha );
}

void InventoryScreen::openForPlacing( const Inventory& inventory, const ItemStack& incoming ) {
	mCursor.openForPlacing( inventory, incoming );
}

bool InventoryScreen::isPlacing() const {
	return mCursor.isPlacing();
}

void InventoryScreen::open() {
	mCursor.open();
}

bool InventoryScreen::back() {
	return mCursor.back();
}

InventoryScreen::Request InventoryScreen::handleInput( Inventory* inventory, ItemId equipped, bool injured, const ItemStack* incoming ) {
	const Request nothing = { Request::NONE, kNone };
	ASSERT( inventory && "InventoryScreen::handleInput : inventory is null." );
	UiInput input = UiInput::read( mPreviousCursor );
	mPreviousCursor = input.cursor;
	const ItemGrid& grid = inventory->grid();
	mCursor.forgetStale( grid );
	layout( grid );
	mCursor.endPlacingIfEmpty( incoming );

	// 入力を読んで、マウスの下の物（項目・マス）と押したキーを、選んでいる状態（InventoryCursor）へ伝える
	if ( mCursor.mode() == InventoryCursor::Mode::Menu ) {
		int pointed = menuAt( grid, input.cursor );
		if ( input.cursorMoved && pointed != kNone ) {
			mCursor.selectMenu( pointed );
		}
		if ( input.upPressed ) {
			mCursor.menuUp();
		}
		if ( input.downPressed ) {
			mCursor.menuDown();
		}
		if ( input.clicked ) {
			if ( pointed == kNone ) {
				mCursor.closeMenu(); // メニューの外を押したら閉じる
				return nothing;
			}
			mCursor.selectMenu( pointed );
			return mCursor.activateMenu( grid );
		}
		if ( input.decidePressed ) {
			return mCursor.activateMenu( grid );
		}
		return nothing;
	}

	// Browse・Moving・Placing は、どれもマスを選ぶ（Moving・Placing は置く所を選ぶ）
	int pointed = mView.cellAt( grid, input.cursor );
	if ( input.cursorMoved ) {
		mCursor.selectCell( pointed );
	}
	mCursor.moveSelection( input.upPressed, input.downPressed, input.leftPressed, input.rightPressed, grid );
	if ( mCursor.isPlacing() && InputQuickPlace() ) {
		return { Request::QUICK_PLACE, kNone };
	}
	bool decided = input.decidePressed;
	if ( input.clicked && pointed != kNone ) {
		mCursor.selectCell( pointed );
		decided = true;
	}
	if ( !decided ) {
		return nothing;
	}
	return mCursor.decide( inventory, equipped, injured, incoming );
}

void InventoryScreen::layout( const ItemGrid& grid ) {
	mView.setLayout( gridOrigin( grid ), kCellSize );
}

UiRect InventoryScreen::menuRect( const ItemGrid& grid, int index ) const {
	Vector2 origin = gridOrigin( grid );
	UiRect rect;
	rect.left = origin.x + gridWidth( grid ) + kMenuGap;
	rect.top = mView.cellRect( grid, mCursor.selected() ).top + index * ( kMenuHeight + kMenuSpacing );
	rect.width = kMenuWidth;
	rect.height = kMenuHeight;
	return rect;
}

int InventoryScreen::menuAt( const ItemGrid& grid, const Vector2& point ) const {
	for ( int i = 0; i < mCursor.menuCount(); ++i ) {
		if ( menuRect( grid, i ).contains( point ) ) {
			return i;
		}
	}
	return kNone;
}

void InventoryScreen::draw( const Inventory& inventory, ItemId equipped, const char* exits, const ItemStack* incoming, PlayerCondition condition ) {
	const ItemGrid& grid = inventory.grid();
	mCursor.forgetStale( grid );
	layout( grid );
	// 板を先に積む（UiRenderer は積んだ順に長方形を描くので、マスより下になる）
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
	mView.drawCells( grid, ( mCursor.mode() == InventoryCursor::Mode::Moving ) ? mCursor.movingFrom() : kNone );
	mView.drawMark( grid, equipped, kEquippedMark );
	drawSelection( inventory, incoming );
	if ( mCursor.mode() == InventoryCursor::Mode::Menu ) {
		drawMenu( grid );
	}
	drawFooter( grid, incoming );
	// 左下に心電図を積む（暗くする板と同じく Framework の画面サイズで位置を決める）
	UiRect ecgArea;
	ecgArea.left = kEcgMargin;
	ecgArea.top = static_cast< float >( Framework::instance().height() ) - kEcgMargin - kEcgHeight;
	ecgArea.width = kEcgWidth;
	ecgArea.height = kEcgHeight;
	mEcg.draw( ecgArea, condition );
	mRenderer->draw();
}

void InventoryScreen::drawSelection( const Inventory& inventory, const ItemStack* incoming ) {
	const ItemGrid& grid = inventory.grid();
	const int selected = mCursor.selected();
	const int movingFrom = mCursor.movingFrom();
	if ( mCursor.mode() == InventoryCursor::Mode::Placing && incoming ) {
		// 入れようとしている所に、拾った物を薄く重ね、その大きさの枠。入らない所は赤（入るのは空き・同じ物に重ねるだけ）
		int target = grid.placementCell( incoming->item, selected );
		bool fits = inventory.countPlaceableInto( selected, *incoming ) > 0;
		mView.drawGhost( grid, target, *incoming );
		mView.addFrame( mView.spanRect( grid, target, itemSpec( incoming->item ).cellSize ), kFrameThickness, fits ? kPlaceColor : kBlockedColor );
		return;
	}
	if ( mCursor.mode() == InventoryCursor::Mode::Moving ) {
		// 置く所に、持ち上げている物の大きさの枠。入らない所は赤
		ItemId item = grid.stackAt( movingFrom ).item;
		int target = grid.placementCell( item, selected );
		bool fits = ( target == movingFrom ) || inventory.canMove( movingFrom, selected );
		mView.addFrame( mView.spanRect( grid, target, itemSpec( item ).cellSize ), kFrameThickness, fits ? kPlaceColor : kBlockedColor );
		return;
	}
	// 物のマスなら、その物全体（2 マスの物は 1 行）に枠
	int owner = grid.ownerCell( selected );
	if ( owner == kNone ) {
		mView.addFrame( mView.cellRect( grid, selected ), kFrameThickness, kSelectColor );
	} else {
		mView.addFrame( mView.spanRect( grid, owner, itemSpec( grid.stackAt( owner ).item ).cellSize ), kFrameThickness, kSelectColor );
	}
}

void InventoryScreen::drawMenu( const ItemGrid& grid ) {
	for ( int i = 0; i < mCursor.menuCount(); ++i ) {
		UiRect rect = menuRect( grid, i );
		bool selected = ( i == mCursor.menuChoice() );
		mRenderer->addRect( rect, selected ? kMenuSelectedColor : kMenuPlateColor );
		mRenderer->addText( kMenuLabels[ mCursor.menuItem( i ) ], rect.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, selected ? kMenuSelectedTextColor : kTextColor );
	}
}

void InventoryScreen::drawFooter( const ItemGrid& grid, const ItemStack* incoming ) {
	const int selected = mCursor.selected();
	const int movingFrom = mCursor.movingFrom();
	const InventoryCursor::Mode mode = mCursor.mode();
	Vector2 position( UiLayout::anchorX( UI_ANCHOR_CENTER ), gridOrigin( grid ).y + gridHeight( grid ) + kFooterGap );
	int owner = grid.ownerCell( selected );
	const char* name = ( owner == kNone ) ? kEmptyName : itemSpec( grid.stackAt( owner ).item ).name;
	char text[ kTextSize ];
	if ( mode == InventoryCursor::Mode::Moving ) {
		name = itemSpec( grid.stackAt( movingFrom ).item ).name; // 持ち上げている物
	} else if ( mode == InventoryCursor::Mode::Placing && incoming ) {
		// 拾って持っている物（重ねられる物は、まだ入れていない数も）
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
	if ( mode == InventoryCursor::Mode::Menu ) {
		hint = kMenuHint;
	} else if ( mode == InventoryCursor::Mode::Placing ) {
		hint = kPlacingHint;
	} else if ( mode == InventoryCursor::Mode::Moving ) {
		hint = kMovingHint;
	}
	position.y += kFooterLineHeight;
	mRenderer->addText( hint, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor );
}
