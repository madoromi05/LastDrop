#include "GameSrc/UI/Widget/UiLabel.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

UiLabel::UiLabel( const char* text, UiAnchor anchor, const Vector2& offset, UiTextSize size, UiAlign align, unsigned color, float letterSpacing ) :
	mText( text ),
	mAnchor( anchor ),
	mOffset( offset ),
	mSize( size ),
	mAlign( align ),
	mColor( color ),
	mLetterSpacing( letterSpacing ) {
	ASSERT( text && "UiLabel : text is null." );
}

void UiLabel::draw( UiRenderer& renderer ) const {
	Vector2 position = mOffset;
	position.x += UiLayout::anchorX( mAnchor );
	renderer.addText( mText.c_str(), position, mSize, mAlign, mColor, mLetterSpacing );
}
