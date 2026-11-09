#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	// 行の中の並び（画素。座標の高さ 480 で。モックの 720 の 2/3）。左から 余白 → ひし形 → 間 → 文字。
	const float kPaddingX = 11.f;
	const float kMarkRadius = 3.5f;   // ひし形の中心から角まで
	const float kMarkToLabel = 12.f;  // ひし形の右の角から文字まで
	const float kLineThickness = 1.f; // 上下の細い線の太さ
	const float kLetterSpacing = 5.f; // 字の間（文字の高さの 0.28 倍くらい。英字の大文字を広く並べる）
}

UiMenuItem::UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate ) :
	mAnchor( anchor ),
	mRect( rect ),
	mLabel( label ),
	mOnActivate( onActivate ),
	mSelected( false ) {
	ASSERT( label && mOnActivate && "UiMenuItem : label or onActivate is empty." );
}

void UiMenuItem::draw( UiRenderer& renderer ) const {
	UiRect rect = screenRect();
	if ( mSelected ) {
		drawSelectedMarks( renderer, rect );
	}
	const float kTwo = 2.f;
	Vector2 labelPosition( rect.left + kPaddingX + kMarkRadius * kTwo + kMarkToLabel, rect.center().y );
	renderer.addText( mLabel.c_str(), labelPosition, UI_TEXT_MENU, UI_ALIGN_MIDDLE_LEFT,
		mSelected ? MenuPalette::TEXT_SELECTED : MenuPalette::TEXT_DIM, kLetterSpacing );
}

void UiMenuItem::drawSelectedMarks( UiRenderer& renderer, const UiRect& rect ) const {
	renderer.addGradientRect( rect, MenuPalette::SELECTED_BAND, MenuPalette::SELECTED_BAND_END );
	UiRect line = rect;
	line.height = kLineThickness;
	renderer.addRect( line, MenuPalette::SELECTED_LINE ); // 上の線
	line.top = rect.bottom() - kLineThickness;
	renderer.addRect( line, MenuPalette::SELECTED_LINE ); // 下の線
	renderer.addDiamond( Vector2( rect.left + kPaddingX + kMarkRadius, rect.center().y ), kMarkRadius, MenuPalette::ACCENT );
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
