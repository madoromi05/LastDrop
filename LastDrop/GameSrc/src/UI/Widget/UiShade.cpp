#include "GameSrc/UI/Widget/UiShade.h"
#include "GameSrc/UI/UiRenderer.h"

#include "GameLib/GameLib.h"
#include "Framework.h"

namespace{
	const unsigned kAlphaMask = 0xff000000u; // VertexColor::pack の並びで a が入る所（上の 1 バイト）
}

UiShade::UiShade( unsigned color, float solidUntil, float fadeUntil ) :
	mColor( color ),
	mSolidUntil( solidUntil ),
	mFadeUntil( fadeUntil ) {
	ASSERT( 0.f <= solidUntil && solidUntil <= fadeUntil && fadeUntil <= 1.f && "UiShade : bad range." );
}

void UiShade::draw( UiRenderer& renderer ) const {
	const float screenWidth = static_cast< float >( GameLib::Framework::instance().width() );
	const float screenHeight = static_cast< float >( GameLib::Framework::instance().height() );
	UiRect solid;
	solid.left = 0.f;
	solid.top = 0.f;
	solid.width = screenWidth * mSolidUntil;
	solid.height = screenHeight;
	renderer.addRect( solid, mColor );
	UiRect fade = solid;
	fade.left = solid.right();
	fade.width = screenWidth * ( mFadeUntil - mSolidUntil );
	const unsigned transparent = mColor & ~kAlphaMask; // 同じ色で a だけ 0
	renderer.addGradientRect( fade, mColor, transparent );
}
