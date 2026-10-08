#include "UiScreen.h"
#include "UiInput.h"
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
}

UiScreen::UiScreen() :
	mPreviousCursor( 0.f, 0.f ) {
}

void UiScreen::handleInput() {
	UiInput input = UiInput::read( mPreviousCursor );
	mPreviousCursor = input.cursor;
	mCanvas.handleInput( input );
}

void UiScreen::draw() {
	mCanvas.draw( mRenderer );
	mRenderer.draw();
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
