#include "GameSrc/Scene/Game/LootScreen.h"
#include "GameSrc/Interaction/ItemGrid.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Core/Pad.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	typedef GridPairPicker::Side Side;
	const Side kBag = Side::Left;   // 左がカバン
	const Side kLoot = Side::Right; // 右が場所の中身
	const int kFirstCell = 0;
	const float kHalf = 0.5f;

	// マス 1 つの一辺（画素）。町のカバンの画面と同じ大きさ（物の絵が同じに見えるように）。
	const float kCellSize = 64.f;
	const float kGridTop = 96.f;       // マス目の上端（座標の高さ 480 に、見出し・カバンの 4 行・下の欄が入る所）
	const float kGridGap = 72.f;       // カバンと場所の間
	const float kGridLabelGap = 20.f;  // マス目の名前の真ん中から、マス目の上端まで
	const float kFooterGap = 22.f;     // 高い方のマス目の下端から、選んだ物の名前の真ん中まで
	const float kFooterLineHeight = 24.f;
	const int kTextSize = 96;

	const char* kBagLabel = "カバン";
	const char* kEmptyName = "空";
	const char* kNameFormat = "%s（%d 点）"; // 選んでいる物の名前と点（1 つあたりの点 × 数）
	const char* kBrowseHint = "[Enter] 持つ   [F] 移す   [Esc] 閉じる";
	const char* kMovingHint = "[Enter] 置く   [Esc] やめる";
	const char* kEquippedMark = "E";

	// 後ろの町を透かして見せる濃さ（町のカバンの画面と同じ 0.5。InventoryPickup_Plan.txt Q6）。
	const float kPanelAlpha = 0.5f;
	const unsigned kDimColor = VertexColor::pack( 0.f, 0.f, 0.f, kPanelAlpha );
	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );
	const unsigned kHintColor = VertexColor::pack( 0.65f, 0.65f, 0.65f );
	const unsigned kHeadingColor = VertexColor::pack( 0.85f, 0.65f, 0.15f ); // カバンの画面の見出しと同じ黄
}

LootScreen::LootScreen( UiRenderer* renderer ) :
	mRenderer( renderer ),
	// カバンは銃 1 種類 1 丁（R2_Plan.txt Q14）。場所の中身は決まり無し（銃を置いていってもよい）。
	mPicker( renderer, &mIcons, GridRule::CarryLimit, GridRule::Free ),
	mPreviousCursor( 0.f, 0.f ) {
	ASSERT( mRenderer && "LootScreen : renderer is null." );
	mPicker.setCellAlpha( kPanelAlpha );
}

void LootScreen::open() {
	mPicker.reset( kLoot, kFirstCell );
}

bool LootScreen::back() {
	return mPicker.back();
}

void LootScreen::layout( const ItemGrid& bag, const ItemGrid& loot ) {
	float bagWidth = ItemGridView::width( bag, kCellSize );
	float lootWidth = ItemGridView::width( loot, kCellSize );
	float left = UiLayout::anchorX( UI_ANCHOR_CENTER ) - ( bagWidth + kGridGap + lootWidth ) * kHalf;
	mPicker.layout( Vector2( left, kGridTop ), Vector2( left + bagWidth + kGridGap, kGridTop ), kCellSize );
}

void LootScreen::handleInput( ItemGrid* bag, ItemGrid* loot ) {
	ASSERT( bag && loot && "LootScreen::handleInput : grid is null." );
	layout( *bag, *loot );
	UiInput input = UiInput::read( mPreviousCursor );
	mPreviousCursor = input.cursor;
	mPicker.forgetStale( *bag, *loot );
	bool onCell = false;
	if ( input.cursorMoved ) {
		onCell = mPicker.point( *bag, *loot, input.cursor );
	} else {
		onCell = mPicker.view( kBag ).cellAt( *bag, input.cursor ) >= 0 || mPicker.view( kLoot ).cellAt( *loot, input.cursor ) >= 0;
	}
	mPicker.moveSelection( input.upPressed, input.downPressed, input.leftPressed, input.rightPressed, *bag, *loot );
	if ( InputQuickPlace() ) {
		mPicker.quickMove( bag, loot );
		return;
	}
	if ( input.decidePressed || ( input.clicked && onCell ) ) {
		mPicker.decide( bag, loot );
	}
}

void LootScreen::draw( const ItemGrid& bag, const ItemGrid& loot, const char* title, ItemId equipped ) {
	layout( bag, loot );
	// 板を先に積む（UiRenderer は積んだ順に長方形を描くので、マスより下になる）。
	UiRect screen;
	screen.left = 0.f;
	screen.top = 0.f;
	screen.width = static_cast< float >( Framework::instance().width() );
	screen.height = static_cast< float >( Framework::instance().height() );
	mRenderer->addRect( screen, kDimColor );

	struct Shown{
		Side side;
		const ItemGrid* grid;
		const char* label;
	};
	const Shown grids[] = { { kBag, &bag, kBagLabel }, { kLoot, &loot, title } };
	for ( const Shown& shown : grids ) {
		UiRect first = mPicker.view( shown.side ).cellRect( *shown.grid, kFirstCell );
		Vector2 label( first.left + ItemGridView::width( *shown.grid, kCellSize ) * kHalf, first.top - kGridLabelGap );
		mRenderer->addText( shown.label, label, UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor );
	}
	mPicker.draw( bag, loot, true );
	mPicker.view( kBag ).drawMark( bag, equipped, kEquippedMark );
	// 空の場所でも「何も無い」は出さない（2026-11-05 ユーザー指定。空のマス目だけで分かる）。
	drawFooter( bag, loot );
	mRenderer->draw();
}

void LootScreen::drawFooter( const ItemGrid& bag, const ItemGrid& loot ) {
	float bagHeight = ItemGridView::height( bag, kCellSize );
	float lootHeight = ItemGridView::height( loot, kCellSize );
	float bottom = kGridTop + ( ( bagHeight > lootHeight ) ? bagHeight : lootHeight );
	Vector2 position( UiLayout::anchorX( UI_ANCHOR_CENTER ), bottom + kFooterGap );
	char text[ kTextSize ];
	const ItemStack* stack = mPicker.focusedStack( bag, loot );
	if ( stack ) {
		const ItemSpec& spec = itemSpec( stack->item );
		std::snprintf( text, sizeof( text ), kNameFormat, spec.name, spec.scoreValue * stack->amount );
	} else {
		std::snprintf( text, sizeof( text ), "%s", kEmptyName );
	}
	mRenderer->addText( text, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	position.y += kFooterLineHeight;
	mRenderer->addText( mPicker.isMoving() ? kMovingHint : kBrowseHint, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor );
}
