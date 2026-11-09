#include "GameSrc/UI/Widget/UiBar.h"
#include "GameSrc/UI/UiRenderer.h"

UiBar::UiBar( UiAnchor anchor, const UiRect& rect, unsigned color ) :
	mAnchor( anchor ),
	mRect( rect ),
	mColor( color ) {
}

void UiBar::draw( UiRenderer& renderer ) const {
	UiRect rect = mRect;
	rect.left += UiLayout::anchorX( mAnchor );
	renderer.addRect( rect, mColor );
}
