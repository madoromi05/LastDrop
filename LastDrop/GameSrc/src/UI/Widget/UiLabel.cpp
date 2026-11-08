#include "GameSrc/UI/Widget/UiLabel.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

UiLabel::UiLabel( const char* text, UiAnchor anchor, const Vector2& offset, UiTextSize size, UiAlign align, unsigned color ) :
	mText( text ),
	mAnchor( anchor ),
	mOffset( offset ),
	mSize( size ),
	mAlign( align ),
	mColor( color ) {
	ASSERT( text && "UiLabel : text is null." );
}

void UiLabel::setText( const char* text ) {
	ASSERT( text && "UiLabel : text is null." );
	mText = text;
}

void UiLabel::setOffset( const Vector2& offset ) {
	mOffset = offset;
}

void UiLabel::draw( UiRenderer& renderer ) const {
	Vector2 position = mOffset;
	position.x += UiLayout::anchorX( mAnchor );
	renderer.addText( mText.c_str(), position, mSize, mAlign, mColor );
}
