#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/UiInput.h"
#include "GameSrc/UI/UiRenderer.h"
#include "Framework.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
	// 縦の位置は画面の高さに対する割合、そこからの間は画素。横は画面の左端から
	const float kSideMarginX = 80.f;            // 題名・ひと言・案内の左端
	const float kSideTitleTopRatio = 0.24f;     // 題名の上端（画面の上から）
	const float kSideTitleToRule = 62.f;        // 題名の上端から赤い線まで（題名の文字の高さ 48 ＋ 間 14）
	const float kSideRuleWidth = 48.f;
	const float kSideRuleThickness = 1.5f;
	const float kSideRuleToTagline = 18.f;      // 赤い線からひと言の縦の真ん中まで
	const float kSideMenuBottomRatio = 0.81f;   // 一番下の行の下端（画面の上から）
	const float kSideMenuItemLeft = 69.f;       // 行の左端（帯と線はここから。文字は行の中の余白の分だけ右）
	const float kSideMenuItemWidth = 253.f;
	const float kSideMenuItemHeight = 35.f;
	const float kSideMenuItemSpacing = 3.f;     // 行と行の間
	// 設定・操作の一覧の見出しと、設定の行
	const float kSideHeadingTopRatio = 0.19f;   // 見出しの上端（画面の上から）
	const float kSideHeadingToRule = 44.f;      // 見出しの上端から赤い線まで（見出しの文字の高さ 32 ＋ 間 12）
	const float kSideHeadingRuleWidth = 37.f;
	const float kSideContentTopRatio = 0.35f;   // 見出しの下の中身の上端（画面の上から）。設定の最初の行・操作の一覧のまとまり
	const float kSideSettingRowWidth = 507.f;   // 名前・バー・数字が並ぶ幅
	const float kSideSettingRowHeight = 40.f;   // スライダーの行（メニューの行より高く、バーを触りやすく）
	const float kSideSettingLinkGap = 19.f;     // 最後のスライダーの行から、ページを開く行までの間
	const float kControlsColumnGap = 37.f;      // 操作の一覧の列と列の間
	const float kSideBackBottomRatio = 0.867f;  // 操作の一覧の「戻る」の行の下端（画面の上から）
	const float kKeyHintBottomMargin = 27.f;    // 画面の下端から案内の縦の真ん中まで
	// 本編の HUD（画素）
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

UiRect UiScreen::sideMenuItemRect( int index, int count ) {
	ASSERT( index >= 0 && index < count && "UiScreen : bad side menu item index." );
	const float pitch = kSideMenuItemHeight + kSideMenuItemSpacing; // 行の上端から次の行の上端まで
	// 行のまとまり全体の高さ。間は行と行のあいだにだけあり、一番下の行の下には無いので 1 つ引く
	const float blockHeight = pitch * count - kSideMenuItemSpacing;
	const float blockTop = Framework::instance().height() * kSideMenuBottomRatio - blockHeight;
	UiRect rect;
	rect.left = kSideMenuItemLeft;
	rect.top = blockTop + index * pitch;
	rect.width = kSideMenuItemWidth;
	rect.height = kSideMenuItemHeight;
	return rect;
}

Vector2 UiScreen::sideHeadingPosition() {
	return Vector2( kSideMarginX, Framework::instance().height() * kSideHeadingTopRatio );
}

UiRect UiScreen::sideHeadingRuleRect() {
	UiRect rect;
	rect.left = kSideMarginX;
	rect.top = sideHeadingPosition().y + kSideHeadingToRule;
	rect.width = kSideHeadingRuleWidth;
	rect.height = kSideRuleThickness;
	return rect;
}

UiRect UiScreen::sideSettingRowRect( int index ) {
	ASSERT( index >= 0 && "UiScreen : bad side setting row index." );
	UiRect rect;
	rect.left = kSideMenuItemLeft;
	rect.top = Framework::instance().height() * kSideContentTopRatio + index * ( kSideSettingRowHeight + kSideMenuItemSpacing );
	rect.width = kSideSettingRowWidth;
	rect.height = kSideSettingRowHeight;
	return rect;
}

UiRect UiScreen::sideSettingLinkRect( int rowCount, int index ) {
	ASSERT( rowCount > 0 && index >= 0 && "UiScreen : bad side setting link index." );
	UiRect rect;
	rect.left = kSideMenuItemLeft;
	rect.top = sideSettingRowRect( rowCount - 1 ).bottom() + kSideSettingLinkGap + index * ( kSideMenuItemHeight + kSideMenuItemSpacing );
	rect.width = kSideSettingRowWidth;
	rect.height = kSideMenuItemHeight;
	return rect;
}

UiColumn UiScreen::controlsColumn( int index, int count ) {
	ASSERT( index >= 0 && index < count && "UiScreen : bad controls column index." );
	UiColumn column;
	column.index = index;
	column.count = count;
	column.margin = kSideMarginX;
	column.gap = kControlsColumnGap;
	return column;
}

float UiScreen::controlsGroupTop() {
	return Framework::instance().height() * kSideContentTopRatio;
}

UiRect UiScreen::sideBackItemRect() {
	UiRect rect;
	rect.left = kSideMenuItemLeft;
	rect.top = Framework::instance().height() * kSideBackBottomRatio - kSideMenuItemHeight;
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
