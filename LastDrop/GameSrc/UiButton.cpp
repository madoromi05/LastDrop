#include "UiButton.h"
#include "UiRenderer.h"
#include "VertexColor.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 板の色。選ばれているかどうかが一目で分かるよう、明るさと不透明度を大きく変える。
	const unsigned kPlateColor = VertexColor::pack( 0.15f, 0.15f, 0.15f, 0.8f );
	const unsigned kSelectedPlateColor = VertexColor::pack( 0.85f, 0.65f, 0.15f, 0.95f ); // 燃料の色（黄）
	const unsigned kLabelColor = VertexColor::pack( 0.9f, 0.9f, 0.9f );
	const unsigned kSelectedLabelColor = VertexColor::pack( 0.05f, 0.05f, 0.05f );
}

UiButton::UiButton( const UiRect& rect, const char* label, std::function< void() > onActivate ) :
	mRect( rect ),
	mLabel( label ),
	mOnActivate( onActivate ),
	mSelected( false ) {
	ASSERT( label && mOnActivate && "UiButton : label or onActivate is empty." );
}

void UiButton::draw( UiRenderer& renderer ) const {
	renderer.addRect( mRect, mSelected ? kSelectedPlateColor : kPlateColor );
	renderer.addText( mLabel.c_str(), mRect.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER,
		mSelected ? kSelectedLabelColor : kLabelColor );
}

bool UiButton::contains( const Vector2& point ) const {
	return mRect.contains( point );
}

void UiButton::setSelected( bool selected ) {
	mSelected = selected;
}

void UiButton::activate() {
	mOnActivate();
}
