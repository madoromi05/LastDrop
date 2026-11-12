#include "GameSrc/UI/Widget/KeyCap.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

namespace {
	// 画素（座標の高さ 480 で）
	const float kFramePaddingX = 5.f;  // 枠の左右の余白
	const float kFramePaddingY = 1.f;  // 枠の上下の余白
	const float kFrameThickness = 1.f; // 枠の線の太さ
	const float kBothSides = 2.f;      // 余白は左右（上下）の 2 か所
	const float kHalf = 0.5f;
	const UiTextSize kTextSize = UI_TEXT_CAPTION;
}

float KeyCap::width( UiRenderer& renderer, const char* key ) {
	ASSERT( key && "KeyCap : key is null." );
	return renderer.textWidth( key, kTextSize ) + kFramePaddingX * kBothSides;
}

float KeyCap::draw( UiRenderer& renderer, const char* key, float left, float middleY ) {
	UiRect frame;
	frame.left = left;
	frame.width = width( renderer, key );
	frame.height = renderer.lineHeight( kTextSize ) + kFramePaddingY * kBothSides;
	frame.top = middleY - frame.height * kHalf;
	// 枠は細い長方形 4 本（上・下・左・右）
	UiRect edge = frame;
	edge.height = kFrameThickness;
	renderer.addRect( edge, MenuPalette::KEY_FRAME );
	edge.top = frame.bottom() - kFrameThickness;
	renderer.addRect( edge, MenuPalette::KEY_FRAME );
	edge = frame;
	edge.width = kFrameThickness;
	renderer.addRect( edge, MenuPalette::KEY_FRAME );
	edge.left = frame.right() - kFrameThickness;
	renderer.addRect( edge, MenuPalette::KEY_FRAME );
	renderer.addText( key, frame.center(), kTextSize, UI_ALIGN_CENTER, MenuPalette::KEY_TEXT );
	return frame.right();
}