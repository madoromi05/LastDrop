#include "GameSrc/UI/Widget/UiKeyHintRow.h"
#include "GameSrc/UI/Widget/KeyCap.h"
#include "GameSrc/UI/Widget/KeyCap.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 画素（座標の高さ 480 で。モックの 720 の 2/3）
	const float kKeyToDescription = 7.f; // 枠の右から説明まで
	const float kHintGap = 19.f;        // 説明の右から次の案内の枠まで
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
	const float descriptionX = KeyCap::draw( renderer, hint.key.c_str(), x, middleY ) + kKeyToDescription;
	renderer.addText( hint.description.c_str(), Vector2( descriptionX, middleY ), kTextSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT_DIM );
	return descriptionX + renderer.textWidth( hint.description.c_str(), kTextSize ) + kHintGap;
}
