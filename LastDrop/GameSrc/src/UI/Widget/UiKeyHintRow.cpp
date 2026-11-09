#include "GameSrc/UI/Widget/UiKeyHintRow.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 画素（座標の高さ 480 で。モックの 720 の 2/3）。
	const float kFramePaddingX = 5.f;   // 枠の左右の余白
	const float kFramePaddingY = 1.f;   // 枠の上下の余白
	const float kFrameThickness = 1.f;  // 枠の線の太さ
	const float kKeyToDescription = 7.f; // 枠の右から説明まで
	const float kHintGap = 19.f;        // 説明の右から次の案内の枠まで
	const float kHalf = 0.5f;
	const float kBothSides = 2.f;       // 余白は左右（上下）の 2 か所
	const UiTextSize kTextSize = UI_TEXT_CAPTION;
}

UiKeyHintRow::UiKeyHintRow( UiAnchor anchor, const Vector2& leftMiddle, const std::vector< Hint >& hints ) :
	mAnchor( anchor ),
	mLeftMiddle( leftMiddle ),
	mHints( hints ) {
	ASSERT( !mHints.empty() && "UiKeyHintRow : hints is empty." );
}

void UiKeyHintRow::draw( UiRenderer& renderer ) const {
	float x = mLeftMiddle.x + UiLayout::anchorX( mAnchor );
	for ( const Hint& hint : mHints ) {
		x = drawHint( renderer, hint, x, mLeftMiddle.y );
	}
}

float UiKeyHintRow::drawHint( UiRenderer& renderer, const Hint& hint, float x, float middleY ) const {
	const float keyWidth = renderer.textWidth( hint.key.c_str(), kTextSize );
	UiRect frame;
	frame.left = x;
	frame.width = keyWidth + kFramePaddingX * kBothSides;
	frame.height = renderer.lineHeight( kTextSize ) + kFramePaddingY * kBothSides;
	frame.top = middleY - frame.height * kHalf;
	// 枠は細い長方形 4 本（上・下・左・右）。
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

	renderer.addText( hint.key.c_str(), frame.center(), kTextSize, UI_ALIGN_CENTER, MenuPalette::KEY_TEXT );
	const float descriptionX = frame.right() + kKeyToDescription;
	renderer.addText( hint.description.c_str(), Vector2( descriptionX, middleY ), kTextSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT_DIM );
	return descriptionX + renderer.textWidth( hint.description.c_str(), kTextSize ) + kHintGap;
}
