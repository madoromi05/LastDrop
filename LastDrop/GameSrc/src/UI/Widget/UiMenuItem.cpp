#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"
#include "GameSrc/UI/Widget/SelectionMarks.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const UiTextSize kTextSize = UI_TEXT_MENU;
}

UiMenuItem::UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate, float letterSpacing,
	UiAlign labelAlign ) :
	mAnchor( anchor ),
	mRect( rect ),
	mLabel( label ),
	mOnActivate( onActivate ),
	mSelected( false ),
	mLetterSpacing( letterSpacing ),
	mLabelAlign( labelAlign ) {
	ASSERT( label && mOnActivate && "UiMenuItem : label or onActivate is empty." );
	ASSERT( ( labelAlign == UI_ALIGN_MIDDLE_LEFT || labelAlign == UI_ALIGN_CENTER ) && "UiMenuItem : labelAlign must be MIDDLE_LEFT or CENTER." );
}

void UiMenuItem::draw( UiRenderer& renderer ) const {
	const UiRect rect = screenRect();
	float labelLeft = 0.f;
	if ( mLabelAlign == UI_ALIGN_CENTER ) {
		const float labelWidth = renderer.textWidth( mLabel.c_str(), kTextSize, mLetterSpacing ); // ひし形の位置が文字の幅で決まるので、先に測る
		if ( mSelected ) {
			SelectionMarks::drawCentered( renderer, rect, labelWidth );
		}
		labelLeft = SelectionMarks::centeredLabelLeft( rect, labelWidth );
	} else {
		if ( mSelected ) {
			SelectionMarks::draw( renderer, rect );
		}
		labelLeft = SelectionMarks::labelLeft( rect );
	}
	renderer.addText( mLabel.c_str(), Vector2( labelLeft, rect.center().y ), kTextSize, UI_ALIGN_MIDDLE_LEFT,
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
