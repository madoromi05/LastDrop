#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"
#include "GameSrc/UI/Widget/SelectionMarks.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

UiMenuItem::UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate , float letterSpacing) :
	mAnchor( anchor ),
	mRect( rect ),
	mLabel( label ),
	mOnActivate( onActivate ),
	mSelected( false ),
	mLetterSpacing(letterSpacing){
	ASSERT( label && mOnActivate && "UiMenuItem : label or onActivate is empty." );
}

void UiMenuItem::draw( UiRenderer& renderer ) const {
	UiRect rect = screenRect();
	if ( mSelected ) {
		SelectionMarks::draw( renderer, rect );
	}

	Vector2 labelPosition( SelectionMarks::labelLeft( rect ) , rect.center().y );
	renderer.addText( mLabel.c_str(), labelPosition, UI_TEXT_MENU, UI_ALIGN_MIDDLE_LEFT,
		mSelected ? MenuPalette::TEXT_SELECTED : MenuPalette::TEXT_DIM, mLetterSpacing );
}

bool UiMenuItem::contains( const Vector2& point ) const {
	return screenRect().contains( point );
}

UiRect UiMenuItem::screenRect() const {
	UiRect rect = mRect;
	rect.left += UiLayout::anchorX( mAnchor );
	return rect;
}

void UiMenuItem::setSelected( bool selected ) {
	mSelected = selected;
}

void UiMenuItem::activate() {
	mOnActivate();
}
