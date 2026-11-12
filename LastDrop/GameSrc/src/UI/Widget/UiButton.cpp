#include "GameSrc/UI/Widget/UiButton.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const unsigned kPlateColor = VertexColor::pack( 0.15f, 0.15f, 0.15f, 0.8f );			// 板の色。選ばれているかどうかが一目で分かるよう、明るさと不透明度を大きく変える
	const unsigned kSelectedPlateColor = VertexColor::pack( 0.85f, 0.65f, 0.15f, 0.95f );	// 燃料の色（黄）
	const unsigned kLabelColor = VertexColor::pack( 0.9f, 0.9f, 0.9f );
	const unsigned kSelectedLabelColor = VertexColor::pack( 0.05f, 0.05f, 0.05f );
}

UiButton::UiButton( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate ) :
	mAnchor( anchor ),
	mRect( rect ),
	mLabel( label ),
	mOnActivate( onActivate ),
	mSelected( false ) {
	ASSERT( label && mOnActivate && "UiButton : label or onActivate is empty." );
}

void UiButton::draw( UiRenderer& renderer ) const {
	UiRect rect = screenRect();
	renderer.addRect( rect, mSelected ? kSelectedPlateColor : kPlateColor );
	renderer.addText( mLabel.c_str(), rect.center(), UI_TEXT_NORMAL, UI_ALIGN_CENTER,
		mSelected ? kSelectedLabelColor : kLabelColor );
}

bool UiButton::contains( const Vector2& point ) const {
	return screenRect().contains( point );
}

UiRect UiButton::screenRect() const {
	UiRect rect = mRect;
	rect.left += UiLayout::anchorX( mAnchor );
	return rect;
}

void UiButton::setSelected( bool selected ) {
	mSelected = selected;
}

void UiButton::activate() {
	mOnActivate();
}
