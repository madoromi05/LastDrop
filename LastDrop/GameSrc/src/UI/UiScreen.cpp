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
	// RE:2 風の左寄せのメニュー（画素。座標の高さ 480 で、モックの 720 の 2/3）。
	// 縦の位置は画面の高さに対する割合、そこからの間は画素。横は画面の左端から。
	const float kSideMarginX = 80.f;            // 題名・ひと言・案内の左端
	const float kSideTitleTopRatio = 0.24f;     // 題名の上端（画面の上から）
	const float kSideTitleToRule = 62.f;        // 題名の上端から赤い線まで（題名の文字の高さ 48 ＋ 間 14）
	const float kSideRuleWidth = 48.f;
	const float kSideRuleThickness = 1.5f;
	const float kSideRuleToTagline = 18.f;      // 赤い線からひと言の縦の真ん中まで
	const float kSideMenuTopRatio = 0.58f;      // 最初の行の上端（画面の上から）
	const float kSideMenuItemLeft = 69.f;       // 行の左端（帯と線はここから。文字は行の中の余白の分だけ右）
	const float kSideMenuItemWidth = 253.f;
	const float kSideMenuItemHeight = 35.f;
	const float kSideMenuItemSpacing = 3.f;     // 行と行の間
	const float kKeyHintBottomMargin = 27.f;    // 画面の下端から案内の縦の真ん中まで
	// 本編の HUD（画素）。
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

Vector2 UiScreen::sideTitlePosition() {
	return Vector2( kSideMarginX, Framework::instance().height() * kSideTitleTopRatio );
}

UiRect UiScreen::sideRuleRect() {
	UiRect rect;
	rect.left = kSideMarginX;
	rect.top = sideTitlePosition().y + kSideTitleToRule;
	rect.width = kSideRuleWidth;
	rect.height = kSideRuleThickness;
	return rect;
}

Vector2 UiScreen::sideTaglinePosition() {
	return Vector2( kSideMarginX, sideRuleRect().bottom() + kSideRuleToTagline );
}

UiRect UiScreen::sideMenuItemRect( int index ) {
	UiRect rect;
	rect.left = kSideMenuItemLeft;
	rect.top = Framework::instance().height() * kSideMenuTopRatio + index * ( kSideMenuItemHeight + kSideMenuItemSpacing );
	rect.width = kSideMenuItemWidth;
	rect.height = kSideMenuItemHeight;
	return rect;
}

Vector2 UiScreen::keyHintPosition() {
	return Vector2( kSideMarginX, Framework::instance().height() - kKeyHintBottomMargin );
}

Vector2 UiScreen::screenCenter() {
	return Vector2( 0.f, Framework::instance().height() * kHalf );
}

Vector2 UiScreen::statusLinePosition( int index ) {
	return Vector2( kStatusMargin, Framework::instance().height() - kStatusMargin - ( index + 1 ) * kStatusLineHeight );
}
