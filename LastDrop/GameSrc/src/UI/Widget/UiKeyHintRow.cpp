#include "GameSrc/UI/Widget/UiKeyHintRow.h"
#include "GameSrc/UI/Widget/KeyCap.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 画素（座標の高さ 480 で。モックの 720 の 2/3）
	const float kKeyToDescription = 7.f; // 枠の右から説明まで
	const float kHintGap = 19.f;        // 説明の右から次の案内の枠まで
	const float kHalf = 0.5f;
	const UiTextSize kTextSize = UI_TEXT_CAPTION;
}

UiKeyHintRow::UiKeyHintRow( UiAnchor anchor, const Vector2& position, const std::vector< Hint >& hints, UiAlign align ) :
	mAnchor( anchor ),
	mPosition( position ),
	mHints( hints ),
	mAlign( align ) {
	ASSERT( !mHints.empty() && "UiKeyHintRow : hints is empty." );
	ASSERT( ( align == UI_ALIGN_MIDDLE_LEFT || align == UI_ALIGN_CENTER ) && "UiKeyHintRow : align must be MIDDLE_LEFT or CENTER." );
}

void UiKeyHintRow::draw( UiRenderer& renderer ) const {
	float x = mPosition.x + UiLayout::anchorX( mAnchor );
	if ( mAlign == UI_ALIGN_CENTER ) {
		x -= rowWidth( renderer ) * kHalf;
	}
	for ( const Hint& hint : mHints ) {
		x = drawHint( renderer, hint, x, mPosition.y );
	}
}

float UiKeyHintRow::drawHint( UiRenderer& renderer, const Hint& hint, float x, float middleY ) const {
	const float descriptionX = KeyCap::draw( renderer, hint.key.c_str(), x, middleY ) + kKeyToDescription;
	renderer.addText( hint.description.c_str(), Vector2( descriptionX, middleY ), kTextSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT_DIM );
	return descriptionX + renderer.textWidth( hint.description.c_str(), kTextSize ) + kHintGap;
}

float UiKeyHintRow::hintWidth( UiRenderer& renderer, const Hint& hint ) const {
	return KeyCap::width( renderer, hint.key.c_str() ) + kKeyToDescription + renderer.textWidth( hint.description.c_str(), kTextSize );
}

float UiKeyHintRow::rowWidth( UiRenderer& renderer ) const {
	float width = 0.f;
	for ( const Hint& hint : mHints ) {
		width += hintWidth( renderer, hint ) + kHintGap;
	}
	return width - kHintGap; // 間は案内と案内のあいだにだけあり、最後の案内の右には無い
}
