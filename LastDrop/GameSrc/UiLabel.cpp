#include "UiLabel.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

UiLabel::UiLabel( const char* text, const Vector2& position, UiTextSize size, UiAlign align, unsigned color ) :
	mText( text ),
	mPosition( position ),
	mSize( size ),
	mAlign( align ),
	mColor( color ) {
	ASSERT( text && "UiLabel : text is null." );
}

void UiLabel::setText( const char* text ) {
	ASSERT( text && "UiLabel : text is null." );
	mText = text;
}

void UiLabel::draw( UiRenderer& renderer ) const {
	renderer.addText( mText.c_str(), mPosition, mSize, mAlign, mColor );
}
