#include "GameSrc/Scene/Base/StashScreen.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/Core/Pad.h"

#include "GameLib/GameLib.h"
#include "Framework.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	typedef GridPairPicker::Side Side;
	const Side kStash = Side::Left; // 左が倉庫
	const Side kBag = Side::Right;  // 右がカバン
	const int kFirstCell = 0;

	const float kHalf = 0.5f;

	const float kCellSize = 40.f;		// マス 1 つの一辺（画素）。座標の高さ 480 に、見出し・倉庫の 8 行・下の欄が入る大きさ
	const float kGridTop = 70.f;		// マス目の上端
	const float kGridGap = 60.f;		// 倉庫とカバンの間
	const float kHeadingTop = 30.f;		// 見出しの真ん中
	const float kGridLabelGap = 14.f;	// マス目の名前の真ん中から、マス目の上端まで
	const float kDeployGap = 28.f;		// カバンの下端から Deploy のボタンの上端まで
	const float kDeployWidth = 140.f;
	const float kDeployHeight = 40.f;
	const float kWarningGap = 22.f;		// ボタンの下端から注意の文字の真ん中まで
	const float kFooterGap = 22.f;		// 倉庫の下端から、選んだ物の名前の真ん中まで
	const float kFooterLineHeight = 22.f;

	const char* kHeading = "倉庫";
	const char* kStashLabel = "倉庫";
	const char* kBagLabel = "カバン";
	const char* kDeployLabel = "出撃";
	const char* kEmptyName = "空";
	const char* kBrowseHint = "[Enter] 持つ   [Tab] 切り替え   [Esc] タイトルへ";
	const char* kMovingHint = "[Enter] 置く   [Esc] やめる";
	const char* kDeployHint = "[Enter] 町へ出る";
	const char* kNoWeaponWarning = "カバンに銃がありません";

	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );
	const unsigned kHintColor = VertexColor::pack( 0.65f, 0.65f, 0.65f );
	const unsigned kHeadingColor = VertexColor::pack( 0.85f, 0.65f, 0.15f ); // ボタンを選んだ色と同じ黄
	const unsigned kButtonColor = VertexColor::pack( 0.15f, 0.15f, 0.15f, 0.9f );
	const unsigned kButtonSelectedColor = VertexColor::pack( 0.85f, 0.65f, 0.15f, 0.95f );
	const unsigned kButtonSelectedTextColor = VertexColor::pack( 0.05f, 0.05f, 0.05f );
	const unsigned kWarningColor = VertexColor::pack( 0.95f, 0.75f, 0.2f );

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
	// 倉庫は何丁でも、カバンは銃 1 種類 1 丁
	mPicker( renderer, &mIcons, GridRule::Free, GridRule::CarryLimit ),
	mOnDeploy( false ),
	mPreviousCursor( 0.f, 0.f ) {
	ASSERT( mRenderer && "StashScreen : renderer is null." );
}

bool StashScreen::back() {
	return mPicker.back();
}

void StashScreen::layout( const ItemGrid& stash, const ItemGrid& bag ) {
	float stashWidth = ItemGridView::width( stash, kCellSize );
	float bagWidth = ItemGridView::width( bag, kCellSize );
	float left = UiLayout::anchorX( UI_ANCHOR_CENTER ) - ( stashWidth + kGridGap + bagWidth ) * kHalf;
	mPicker.layout( Vector2( left, kGridTop ), Vector2( left + stashWidth + kGridGap, kGridTop ), kCellSize );
}

UiRect StashScreen::deployRect( const ItemGrid& bag ) const {
	const ItemGridView& bagView = mPicker.view( kBag );
	UiRect first = bagView.cellRect( bag, kFirstCell );
	float bagWidth = ItemGridView::width( bag, kCellSize );
	UiRect rect;
	rect.width = kDeployWidth;
	rect.height = kDeployHeight;
	rect.left = first.left + ( bagWidth - kDeployWidth ) * kHalf;
	rect.top = first.top + ItemGridView::height( bag, kCellSize ) + kDeployGap;
	return rect;
}

void StashScreen::cycleFocus() {
	if ( mOnDeploy ) {
		mOnDeploy = false;
		mPicker.select( kStash, kFirstCell );
	} else if ( mPicker.focus() == kStash ) {
		mPicker.select( kBag, kFirstCell );
	} else {
		mOnDeploy = true;
	}
}

StashScreen::Request StashScreen::handleInput( ItemGrid* stash, ItemGrid* bag ) {
	ASSERT( stash && bag && "StashScreen::handleInput : grid is null." );
	layout( *stash, *bag );
	UiInput input = UiInput::read( mPreviousCursor );
	mPreviousCursor = input.cursor;
	mPicker.forgetStale( *stash, *bag );	// F6 で中身が変わっても、消えた物を持ち上げたままにしない

	// マウス：カーソルの下を選ぶ
	bool onDeploy = deployRect( *bag ).contains( input.cursor );
	bool onCell = false;
	if ( input.cursorMoved ) {
		onCell = mPicker.point( *stash, *bag, input.cursor );
		if ( onCell ) {
			mOnDeploy = false;
		} else if ( onDeploy ) {
			mOnDeploy = true;
		}
	} else {
		onCell = mPicker.view( kStash ).cellAt( *stash, input.cursor ) >= 0 || mPicker.view( kBag ).cellAt( *bag, input.cursor ) >= 0;
	}
	if ( InputInventory() ) {
		cycleFocus();
	}
	if ( mOnDeploy ) {
		if ( input.upPressed ) {
			mOnDeploy = false;
			mPicker.select( kBag, bag->cellCount() - 1 ); // カバンの一番下へ戻る
		}
	} else if ( mPicker.moveSelection( input.upPressed, input.downPressed, input.leftPressed, input.rightPressed,
		*stash, *bag ) == GridPairPicker::Exit::BelowRight ) {
		mOnDeploy = true; // カバンの一番下から Deploy へ
	}
	bool clickedSomething = input.clicked && ( onCell || onDeploy );
	if ( !input.decidePressed && !clickedSomething ) {
		return Request::None;
	}
	if ( mOnDeploy ) {
		return mPicker.isMoving() ? Request::None : Request::Deploy; // 持ち上げたまま出撃しない（置いてから）
	}
	mPicker.decide( stash, bag );
	return Request::None;
}

void StashScreen::draw( const ItemGrid& stash, const ItemGrid& bag ) {
	layout( stash, bag );
	float center = UiLayout::anchorX( UI_ANCHOR_CENTER );
	mRenderer->addText( kHeading, Vector2( center, kHeadingTop ), UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor );

	struct Shown{
		Side side;
		const ItemGrid* grid;
		const char* label;
	};
	const Shown grids[] = { { kStash, &stash, kStashLabel }, { kBag, &bag, kBagLabel } };
	for ( const Shown& shown : grids ) {
		UiRect first = mPicker.view( shown.side ).cellRect( *shown.grid, kFirstCell );
		Vector2 label( first.left + ItemGridView::width( *shown.grid, kCellSize ) * kHalf, first.top - kGridLabelGap );
		mRenderer->addText( shown.label, label, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	}
	mPicker.draw( stash, bag, !mOnDeploy ); // Deploy を選んでいる間は、ボタンの色で見せる
	drawDeploy( bag );
	drawFooter( stash, bag );
	mRenderer->draw();
}

void StashScreen::drawDeploy( const ItemGrid& bag ) {
	UiRect rect = deployRect( bag );
	mRenderer->addRect( rect, mOnDeploy ? kButtonSelectedColor : kButtonColor );
	mRenderer->addText( kDeployLabel, rect.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER, mOnDeploy ? kButtonSelectedTextColor : kTextColor );
	// カバンに銃が無ければ知らせる（止めはしない）
	if ( !hasWeapon( bag ) ) {
		Vector2 line( rect.center().x, rect.bottom() + kWarningGap );
		mRenderer->addText( kNoWeaponWarning, line, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kWarningColor );
	}
}

void StashScreen::drawFooter( const ItemGrid& stash, const ItemGrid& bag ) {
	float bottom = mPicker.view( kStash ).cellRect( stash, kFirstCell ).top + ItemGridView::height( stash, kCellSize );
	Vector2 position( UiLayout::anchorX( UI_ANCHOR_CENTER ), bottom + kFooterGap );
	bool moving = mPicker.isMoving();
	if ( !mOnDeploy || moving ) {
		const ItemStack* stack = mPicker.focusedStack( stash, bag );
		const char* name = stack ? itemSpec( stack->item ).name : kEmptyName; // 銃の弾数はマスの左下に出す
		mRenderer->addText( name, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );
	}
	const char* hint = moving ? kMovingHint : ( mOnDeploy ? kDeployHint : kBrowseHint );
	position.y += kFooterLineHeight;
	mRenderer->addText( hint, position, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor );
}
