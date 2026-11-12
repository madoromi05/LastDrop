#include "GameSrc/UI/Widget/UiTabBar.h"
#include "GameSrc/UI/Widget/SelectionMarks.h"
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/MenuPalette.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const float kLetterSpacing = UiMenuItem::kJapaneseSpacing;	// タブの名前は日本語なので、メニューの行と同じく詰める
	const float kUnderlineInset = 3.f;		// 今のタブの下の赤い線の、行の下端からの高さ
	const float kUnderlineThickness = 1.5f;	// 見出しの下の赤い線と同じ太さ
}

UiTabBar::UiTabBar( const UiRect& rect, const std::vector< std::string >& labels, int current, float tabWidth, UiTextSize textSize,
	std::function< void( int ) > onChange ) :
	mRect( rect ),
	mLabels( labels ),
	mCurrent( current ),
	mTabWidth( tabWidth ),
	mTextSize( textSize ),
	mOnChange( onChange ),
	mSelected( false ) {
	ASSERT( !mLabels.empty() && mOnChange && "UiTabBar : labels or onChange is empty." );
	ASSERT( mCurrent >= 0 && mCurrent < static_cast< int >( mLabels.size() ) && "UiTabBar : bad current tab." );
}

void UiTabBar::draw( UiRenderer& renderer ) const {
	if ( mSelected ) {
		SelectionMarks::draw( renderer, mRect );
	}
	const float middleY = mRect.center().y;
	const int count = static_cast< int >( mLabels.size() );
	for ( int i = 0; i < count; ++i ) {
		const char* label = mLabels[ i ].c_str();
		const bool current = ( i == mCurrent );
		const float left = tabLeft( i );
		renderer.addText( label, Vector2( left, middleY ), mTextSize, UI_ALIGN_MIDDLE_LEFT,
			current ? MenuPalette::TEXT_SELECTED : MenuPalette::TEXT_DIM, kLetterSpacing );
		if ( current ) {
			UiRect underline;
			underline.left = left;
			underline.top = mRect.bottom() - kUnderlineInset;
			underline.width = renderer.textWidth( label, mTextSize, kLetterSpacing );
			underline.height = kUnderlineThickness;
			renderer.addRect( underline, MenuPalette::ACCENT );
		}
	}
}

bool UiTabBar::contains( const Vector2& point ) const {
	return mRect.contains( point );
}

void UiTabBar::setSelected( bool selected ) {
	mSelected = selected;
}

void UiTabBar::activate() {
	adjust( 1 );
}

void UiTabBar::adjust( int direction ) {
	const int count = static_cast< int >( mLabels.size() );
	change( ( mCurrent + direction + count ) % count );
}

void UiTabBar::dragTo( const Vector2& cursor ) {
	if ( cursor.x < tabLeft( 0 ) ) {
		return;
	}
	const int index = static_cast< int >( ( cursor.x - tabLeft( 0 ) ) / mTabWidth );
	if ( index < static_cast< int >( mLabels.size() ) ) {
		change( index );
	}
}

void UiTabBar::change( int index ) {
	if ( index == mCurrent ) {
		return;
	}
	mCurrent = index;
	mOnChange( mCurrent );
}

float UiTabBar::tabLeft( int index ) const {
	return SelectionMarks::labelLeft( mRect ) + index * mTabWidth;
}
