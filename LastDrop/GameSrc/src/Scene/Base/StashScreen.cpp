#include "GameSrc/Scene/Base/StashScreen.h"
#include "GameSrc/Raid/RunState.h"
#include "GameSrc/Interaction/ItemTransfer.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Core/Pad.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <algorithm>
#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
	const float kTwice = 2.f; // 画面の右端 = 横の中央の 2 倍
	const int kNone = -1;

	// マス 1 つの一辺（画素）。座標の高さ 480 に、見出し・倉庫の 8 行・下の欄が入る大きさ（R2_Plan.txt 6-1）。
	const float kCellSize = 40.f;
	const float kGridTop = 70.f;        // マス目の上端
	const float kGridGap = 60.f;        // 倉庫とカバンの間
	const float kHeadingTop = 30.f;     // 見出し「BASE」の真ん中
	const float kGridLabelGap = 14.f;   // マス目の名前の真ん中から、マス目の上端まで
	const float kDayFromRight = 90.f;   // 「DAY 2 / 5」の真ん中の、画面の右端からの長さ
	const float kBestLineGap = 24.f;    // 「DAY」の行から「BEST」の行まで
	const float kDeployGap = 28.f;      // カバンの下端から Deploy のボタンの上端まで
	const float kDeployWidth = 140.f;
	const float kDeployHeight = 40.f;
	const float kWarningGap = 22.f;     // ボタンの下端から注意の文字の真ん中まで
	const float kFooterGap = 22.f;      // 倉庫の下端から、選んだ物の名前の真ん中まで
	const float kFooterLineHeight = 22.f;
	const float kFrameThickness = 2.f;
	const int kTextSize = 64;

	const char* kHeading = "拠点";
	const char* kStashLabel = "倉庫";
	const char* kBagLabel = "カバン";
	const char* kDeployLabel = "出撃";
	const char* kDayFormat = "DAY %d / %d";
	const char* kBestFormat = "最高 %d";
	const char* kEmptyName = "空";
	const char* kBrowseHint = "[Enter] 持つ   [Tab] 切り替え   [Esc] タイトルへ";
	const char* kMovingHint = "[Enter] 置く   [Esc] やめる";
	const char* kDeployHint = "[Enter] 町へ出る";
	const char* kNoWeaponWarning = "カバンに銃がありません";

	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );
	const unsigned kHintColor = VertexColor::pack( 0.65f, 0.65f, 0.65f );
	const unsigned kHeadingColor = VertexColor::pack( 0.85f, 0.65f, 0.15f ); // ボタンを選んだ色と同じ黄
	const unsigned kSelectColor = VertexColor::pack( 0.95f, 0.85f, 0.3f );
	const unsigned kPlaceColor = VertexColor::pack( 0.35f, 0.9f, 0.35f );   // 置ける所
	const unsigned kBlockedColor = VertexColor::pack( 0.9f, 0.2f, 0.15f );  // 入らない所
	const unsigned kButtonColor = VertexColor::pack( 0.15f, 0.15f, 0.15f, 0.9f );
	const unsigned kButtonSelectedColor = VertexColor::pack( 0.85f, 0.65f, 0.15f, 0.95f );
	const unsigned kButtonSelectedTextColor = VertexColor::pack( 0.05f, 0.05f, 0.05f );
	const unsigned kWarningColor = VertexColor::pack( 0.95f, 0.75f, 0.2f );

	// マス目ごとの「入れてよい物」の決まり（倉庫は何丁でも、カバンは銃 1 種類 1 丁。R2_Plan.txt Q14）。
	GridRule ruleOf( bool isBag ) {
		return isBag ? GridRule::CarryLimit : GridRule::Free;
	}

	bool hasWeapon( const ItemGrid& grid ) {
		for ( int i = 0; i < WEAPON_NUMBER; ++i ) {
			if ( grid.total( weaponSpec( static_cast< WeaponId >( i ) ).item ) > 0 ) {
				return true;
			}
		}
		return false;
	}
}

StashScreen::StashScreen( UiRenderer* renderer ) :
	mRenderer( renderer ),
	mStashView( renderer, &mIcons ),
	mBagView( renderer, &mIcons ),
	mFocus( Focus::Stash ),
	mSelected( 0 ),
	mMoving( false ),
	mMovingGrid( Focus::Stash ),
	mMovingFrom( kNone ),
	mPreviousCursor( 0.f, 0.f ) {
	ASSERT( mRenderer && "StashScreen : renderer is null." );
}

bool StashScreen::back() {
	if ( !mMoving ) {
		return false;
	}
	mMoving = false;
	return true;
}

const ItemGrid& StashScreen::grid( const RunState& run, Focus focus ) {
	return ( focus == Focus::Bag ) ? run.bag() : run.stash();
}

ItemGrid& StashScreen::grid( RunState* run, Focus focus ) {
	return ( focus == Focus::Bag ) ? run->bag() : run->stash();
}

ItemGridView& StashScreen::view( Focus focus ) {
	return ( focus == Focus::Bag ) ? mBagView : mStashView;
}

const ItemGridView& StashScreen::view( Focus focus ) const {
	return ( focus == Focus::Bag ) ? mBagView : mStashView;
}

void StashScreen::layout( const RunState& run ) {
	float stashWidth = ItemGridView::width( run.stash(), kCellSize );
	float bagWidth = ItemGridView::width( run.bag(), kCellSize );
	float left = UiLayout::anchorX( UI_ANCHOR_CENTER ) - ( stashWidth + kGridGap + bagWidth ) * kHalf;
	mStashView.setLayout( Vector2( left, kGridTop ), kCellSize );
	mBagView.setLayout( Vector2( left + stashWidth + kGridGap, kGridTop ), kCellSize );
}

UiRect StashScreen::deployRect( const RunState& run ) const {
	UiRect bag = mBagView.cellRect( run.bag(), 0 );
	float bagWidth = ItemGridView::width( run.bag(), kCellSize );
	UiRect rect;
	rect.width = kDeployWidth;
	rect.height = kDeployHeight;
	rect.left = bag.left + ( bagWidth - kDeployWidth ) * kHalf;
	rect.top = bag.top + ItemGridView::height( run.bag(), kCellSize ) + kDeployGap;
	return rect;
}

StashScreen::Request StashScreen::handleInput( RunState* run ) {
	ASSERT( run && "StashScreen::handleInput : run is null." );
	layout( *run );
	UiInput input = UiInput::read( mPreviousCursor );
	mPreviousCursor = input.cursor;
	// F6 で中身が変わっても、消えた物を持ち上げたままにしない。
	if ( mMoving && grid( *run, mMovingGrid ).ownerCell( mMovingFrom ) != mMovingFrom ) {
		mMoving = false;
	}

	// マウス：カーソルの下を選ぶ。
	int stashCell = mStashView.cellAt( run->stash(), input.cursor );
	int bagCell = mBagView.cellAt( run->bag(), input.cursor );
	bool onDeploy = deployRect( *run ).contains( input.cursor );
	if ( input.cursorMoved ) {
		if ( stashCell != kNone ) {
			mFocus = Focus::Stash;
			mSelected = stashCell;
		} else if ( bagCell != kNone ) {
			mFocus = Focus::Bag;
			mSelected = bagCell;
		} else if ( onDeploy ) {
			mFocus = Focus::Deploy;
		}
	}
	// Tab：倉庫 → カバン → Deploy の順に移る。
	if ( InputInventory() ) {
		mFocus = ( mFocus == Focus::Stash ) ? Focus::Bag : ( ( mFocus == Focus::Bag ) ? Focus::Deploy : Focus::Stash );
		mSelected = 0;
	}
	moveSelection( input.upPressed, input.downPressed, input.leftPressed, input.rightPressed, *run );
	bool clickedSomething = input.clicked && ( stashCell != kNone || bagCell != kNone || onDeploy );
	if ( input.decidePressed || clickedSomething ) {
		return decide( run );
	}
	return Request::None;
}

void StashScreen::moveSelection( bool up, bool down, bool left, bool right, const RunState& run ) {
	if ( mFocus == Focus::Deploy ) {
		if ( up ) {
			mFocus = Focus::Bag;
			mSelected = run.bag().cellCount() - 1;
		}
		return;
	}
	const ItemGrid& current = grid( run, mFocus );
	int row = mSelected / current.columns();
	int column = mSelected % current.columns();
	if ( up ) {
		row = std::max( row - 1, 0 );
	}
	if ( down ) {
		if ( mFocus == Focus::Bag && row == current.rows() - 1 ) {
			mFocus = Focus::Deploy; // カバンの一番下から Deploy へ
			return;
		}
		row = std::min( row + 1, current.rows() - 1 );
	}
	if ( left ) {
		if ( mFocus == Focus::Bag && column == 0 ) {
			// カバンの左端から倉庫の右端へ（同じ行。倉庫の方が行が多い）。
			mFocus = Focus::Stash;
			mSelected = std::min( row, run.stash().rows() - 1 ) * run.stash().columns() + run.stash().columns() - 1;
			return;
		}
		column = std::max( column - 1, 0 );
	}
	if ( right ) {
		if ( mFocus == Focus::Stash && column == current.columns() - 1 ) {
			// 倉庫の右端からカバンの左端へ（カバンの行に収める）。
			mFocus = Focus::Bag;
			mSelected = std::min( row, run.bag().rows() - 1 ) * run.bag().columns();
			return;
		}
		column = std::min( column + 1, current.columns() - 1 );
	}
	mSelected = row * current.columns() + column;
}

StashScreen::Request StashScreen::decide( RunState* run ) {
	if ( mFocus == Focus::Deploy ) {
		return mMoving ? Request::None : Request::Deploy; // 持ち上げたまま出撃しない（置いてから）
	}
	ItemGrid& current = grid( run, mFocus );
	if ( !mMoving ) {
		int owner = current.ownerCell( mSelected );
		if ( owner != kNone ) {
			mMoving = true;
			mMovingGrid = mFocus;
			mMovingFrom = owner;
		}
		return Request::None;
	}
	ItemGrid& source = grid( run, mMovingGrid );
	// 元の場所を選んだら取りやめ（動かさない）。
	if ( mMovingGrid == mFocus && source.placementCell( source.stackAt( mMovingFrom ).item, mSelected ) == mMovingFrom ) {
		mMoving = false;
		return Request::None;
	}
	if ( !canPlace( *run ) ) {
		return Request::None; // 入らない所。赤い枠のまま、別の所を選んでもらう
	}
	ItemTransfer::move( &source, mMovingFrom, &current, mSelected );
	mMoving = false;
	return Request::None;
}

bool StashScreen::canPlace( const RunState& run ) const {
	const ItemGrid& source = grid( run, mMovingGrid );
	const ItemGrid& target = grid( run, mFocus );
	if ( mMovingGrid == mFocus && source.placementCell( source.stackAt( mMovingFrom ).item, mSelected ) == mMovingFrom ) {
		return true; // 取りやめ
	}
	return ItemTransfer::canMove( source, ruleOf( mMovingGrid == Focus::Bag ), mMovingFrom, target, ruleOf( mFocus == Focus::Bag ), mSelected );
}

void StashScreen::draw( const RunState& run ) {
	layout( run );
	float center = UiLayout::anchorX( UI_ANCHOR_CENTER );
	char text[ kTextSize ];
	mRenderer->addText( kHeading, Vector2( center, kHeadingTop ), UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor );
	std::snprintf( text, sizeof( text ), kDayFormat, run.day(), RunState::kDayCount );
	Vector2 dayLine( center * kTwice - kDayFromRight, kHeadingTop );
	mRenderer->addText( text, dayLine, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	std::snprintf( text, sizeof( text ), kBestFormat, run.bestScore() );
	mRenderer->addText( text, Vector2( dayLine.x, dayLine.y + kBestLineGap ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor );

	struct Shown{
		Focus focus;
		const char* label;
	};
	const Shown grids[] = { { Focus::Stash, kStashLabel }, { Focus::Bag, kBagLabel } };
	for ( const Shown& shown : grids ) {
		const ItemGrid& g = grid( run, shown.focus );
		ItemGridView& v = view( shown.focus );
		UiRect first = v.cellRect( g, 0 );
		Vector2 label( first.left + ItemGridView::width( g, kCellSize ) * kHalf, first.top - kGridLabelGap );
		mRenderer->addText( shown.label, label, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
		bool liftedHere = mMoving && mMovingGrid == shown.focus;
		v.drawCells( g, liftedHere ? mMovingFrom : kNone );
	}
	drawSelection( run );
	drawDeploy( run );
	drawFooter( run );
	mRenderer->draw();
}

void StashScreen::drawSelection( const RunState& run ) {
	if ( mFocus == Focus::Deploy ) {
		return; // ボタンの色で見せる
	}
	const ItemGrid& g = grid( run, mFocus );
	ItemGridView& v = view( mFocus );
	if ( mMoving ) {
		// 置く所に、持ち上げている物の大きさの枠。入らない所は赤。
		ItemId item = grid( run, mMovingGrid ).stackAt( mMovingFrom ).item;
		int target = g.placementCell( item, mSelected );
		v.addFrame( v.spanRect( g, target, itemSpec( item ).cellSize ), kFrameThickness, canPlace( run ) ? kPlaceColor : kBlockedColor );
		return;
	}
	int owner = g.ownerCell( mSelected );
	if ( owner == kNone ) {
		v.addFrame( v.cellRect( g, mSelected ), kFrameThickness, kSelectColor );
	} else {
		v.addFrame( v.spanRect( g, owner, itemSpec( g.stackAt( owner ).item ).cellSize ), kFrameThickness, kSelectColor );
	}
}

void StashScreen::drawDeploy( const RunState& run ) {
	UiRect rect = deployRect( run );
	bool selected = ( mFocus == Focus::Deploy );
	mRenderer->addRect( rect, selected ? kButtonSelectedColor : kButtonColor );
	mRenderer->addText( kDeployLabel, rect.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, selected ? kButtonSelectedTextColor : kTextColor );
	// カバンに銃が無ければ知らせる（止めはしない。R2_Plan.txt 11 章。タダの拳銃は 11-05 にやめた。InventoryPickup_Plan.txt Q1）。
	if ( !hasWeapon( run.bag() ) ) {
		Vector2 line( rect.center().x, rect.bottom() + kWarningGap );
		mRenderer->addText( kNoWeaponWarning, line, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kWarningColor );
	}
}

void StashScreen::drawFooter( const RunState& run ) {
	float bottom = mStashView.cellRect( run.stash(), 0 ).top + ItemGridView::height( run.stash(), kCellSize );
	Vector2 position( UiLayout::anchorX( UI_ANCHOR_CENTER ), bottom + kFooterGap );
	const char* name = kEmptyName;
	const ItemStack* stack = nullptr;
	if ( mMoving ) {
		stack = &grid( run, mMovingGrid ).stackAt( mMovingFrom ); // 持ち上げている物
	} else if ( mFocus != Focus::Deploy ) {
		const ItemGrid& g = grid( run, mFocus );
		int owner = g.ownerCell( mSelected );
		stack = ( owner == kNone ) ? nullptr : &g.stackAt( owner );
	}
	if ( stack ) {
		name = itemSpec( stack->item ).name; // 銃の弾数はマスの左下に出す（InventoryPickup_Plan.txt Q2）
	}
	if ( mFocus != Focus::Deploy || mMoving ) {
		mRenderer->addText( name, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	}
	const char* hint = mMoving ? kMovingHint : ( ( mFocus == Focus::Deploy ) ? kDeployHint : kBrowseHint );
	position.y += kFooterLineHeight;
	mRenderer->addText( hint, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor );
}
