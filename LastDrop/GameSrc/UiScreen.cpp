#include "UiScreen.h"
#include "UiInput.h"
#include "UiRenderer.h"
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
	Framework framework = Framework::instance();
	return Vector2( framework.width() * kHalf, framework.height() * kHeadingHeightRatio );
}

UiRect UiScreen::menuButtonRect( int index ) {
	Framework framework = Framework::instance();
	UiRect rect;
	rect.width = kMenuButtonWidth;
	rect.height = kMenuButtonHeight;
	rect.left = framework.width() * kHalf - kMenuButtonWidth * kHalf;
	rect.top = framework.height() * kHalf + kMenuTopOffset + index * ( kMenuButtonHeight + kMenuButtonSpacing );
	return rect;
}

Vector2 UiScreen::screenCenter() {
	Framework framework = Framework::instance();
	return Vector2( framework.width() * kHalf, framework.height() * kHalf );
}

Vector2 UiScreen::promptPosition() {
	Vector2 position = screenCenter();
	position.y += kPromptOffsetY;
	return position;
}

Vector2 UiScreen::statusLinePosition( int index ) {
	Framework framework = Framework::instance();
	return Vector2( kStatusMargin, framework.height() - kStatusMargin - ( index + 1 ) * kStatusLineHeight );
}
