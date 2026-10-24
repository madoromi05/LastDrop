#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiRenderer.h"
#include "Framework.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
	// 見出しは画面の上から 1/3 の所。
	const float kHeadingHeightRatio = 1.f / 3.f;
	// ボタンの大きさと並べ方（画素）。最初のボタンは画面の縦の中央より少し下から始める。
	const float kMenuButtonWidth = 220.f;
	const float kMenuButtonHeight = 44.f;
	const float kMenuButtonSpacing = 14.f;  // ボタンとボタンの間
	const float kMenuTopOffset = 20.f;      // 画面の縦の中央から、最初のボタンの上端まで

	// 設定画面の行（画素）。高さと間はメニューのボタンと同じ。行が多いので、メニューより上（縦の中央の少し上）から始める。
	const float kSettingRowWidth = 560.f;
	const float kSettingTopOffset = -60.f;  // 画面の縦の中央から、最初の行の上端まで

	// 操作の一覧（画素）。見出しの下から 2 列で並べる。
	const float kListTopOffset = 60.f;      // 見出しから最初の行の真ん中まで
	const float kListLineHeight = 28.f;     // UI_TEXT_NORMAL の文字より少し広く
	const float kListColumnX[] = { -220.f, 20.f }; // 列ごとの左端（画面の横の中央から）。操作の名前・キーの名前
	const float kListButtonGap = 24.f;      // 最後の行の真ん中から、ボタンの上端まで
	// 本編の HUD（画素）。
	const float kPromptOffsetY = 40.f;     // 照準から案内までの下向きの距離
	const float kStatusMargin = 16.f;      // 画面の左端・下端からの余白
	const float kStatusLineHeight = 26.f;  // 状態表示1行の高さ（UI_TEXT_NORMAL の文字より少し広く）
}

UiScreen::UiScreen( UiRenderer* renderer ) :
	mRenderer( renderer ),
	mPreviousCursor( 0.f, 0.f ) {
	ASSERT( mRenderer && "UiScreen : renderer is null." );
}

void UiScreen::handleInput() {
	UiInput input = UiInput::read( mPreviousCursor );
	mPreviousCursor = input.cursor;
	mCanvas.handleInput( input );
}

void UiScreen::draw() {
	mCanvas.draw( *mRenderer );
	mRenderer->draw();
}

Vector2 UiScreen::headingPosition() {
	return Vector2( 0.f, Framework::instance().height() * kHeadingHeightRatio );
}

UiRect UiScreen::menuButtonRect( int index ) {
	UiRect rect;
	rect.width = kMenuButtonWidth;
	rect.height = kMenuButtonHeight;
	rect.left = -kMenuButtonWidth * kHalf;
	rect.top = Framework::instance().height() * kHalf + kMenuTopOffset + index * ( kMenuButtonHeight + kMenuButtonSpacing );
	return rect;
}

UiRect UiScreen::settingRowRect( int index ) {
	UiRect rect;
	rect.width = kSettingRowWidth;
	rect.height = kMenuButtonHeight;
	rect.left = -kSettingRowWidth * kHalf;
	rect.top = Framework::instance().height() * kHalf + kSettingTopOffset + index * ( kMenuButtonHeight + kMenuButtonSpacing );
	return rect;
}

UiRect UiScreen::settingButtonRect( int index ) {
	UiRect rect = settingRowRect( index );
	rect.width = kMenuButtonWidth;
	rect.left = -kMenuButtonWidth * kHalf;
	return rect;
}

Vector2 UiScreen::listLinePosition( int index, int column ) {
	ASSERT( column >= 0 && column < static_cast< int >( sizeof( kListColumnX ) / sizeof( kListColumnX[ 0 ] ) ) && "UiScreen : bad list column." );
	return Vector2( kListColumnX[ column ], headingPosition().y + kListTopOffset + index * kListLineHeight );
}

UiRect UiScreen::listButtonRect( int lineCount ) {
	UiRect rect;
	rect.width = kMenuButtonWidth;
	rect.height = kMenuButtonHeight;
	rect.left = -kMenuButtonWidth * kHalf;
	rect.top = listLinePosition( lineCount - 1, 0 ).y + kListButtonGap;
	return rect;
}

Vector2 UiScreen::screenCenter() {
	return Vector2( 0.f, Framework::instance().height() * kHalf );
}

Vector2 UiScreen::promptPosition() {
	Vector2 position = screenCenter();
	position.y += kPromptOffsetY;
	return position;
}

Vector2 UiScreen::statusLinePosition( int index ) {
	return Vector2( kStatusMargin, Framework::instance().height() - kStatusMargin - ( index + 1 ) * kStatusLineHeight );
}
