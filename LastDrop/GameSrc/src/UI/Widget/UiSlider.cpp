#include "GameSrc/UI/Widget/UiSlider.h"
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/Widget/SelectionMarks.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include <algorithm>
#include <cmath>

using namespace GameLib::Math;

namespace{
	// 行の中の並べ方（画素。座標の高さ 480 で、モックの 720 の 2/3）。左から 印 → 名前 → バー → 数字
	const float kLabelWidth = 147.f;     // 名前の欄の幅（一番長い「効果音の音量」が収まる幅）
	const float kColumnGap = 12.f;       // 名前とバー、バーと数字の間
	const float kValueWidth = 48.f;      // 数字の欄の幅（"1.25" が収まる幅。数字は欄の右に寄せる）
	const float kPaddingRight = 11.f;    // 行の右端から数字の右端まで（左の余白と同じ）
	const float kBarThickness = 1.5f;    // バーの線の太さ
	const float kBarHitHeight = 16.f;    // バーをクリックできる縦の幅（線は細いので、上下に広げて取る）
	const float kThumbRadius = 5.5f;     // 今の値の所のひし形（中心から角まで）
	const float kHalf = 0.5f;
	const UiTextSize kTextSize = UI_TEXT_MENU;
}

UiSlider::UiSlider( UiAnchor anchor, const UiRect& rect, const char* label, const Range& range,
	std::function< int() > getValue, std::function< void( int ) > setValue, std::function< std::string( int ) > formatValue ) :
	mAnchor( anchor ),
	mRect( rect ),
	mLabel( label ),
	mRange( range ),
	mGetValue( getValue ),
	mSetValue( setValue ),
	mFormatValue( formatValue ),
	mSelected( false ) {
	ASSERT( label && mGetValue && mSetValue && mFormatValue && "UiSlider : label or functions are empty." );
	ASSERT( mRange.minValue < mRange.maxValue && mRange.step > 0 && "UiSlider : bad range." );
}

void UiSlider::draw( UiRenderer& renderer ) const {
	UiRect row = screenRect();
	if ( mSelected ) {
		SelectionMarks::draw( renderer, row );
	}
	// 名前は日本語なので、メニューの日本語の行（操作の一覧・戻る）と同じ字の間にする
	renderer.addText( mLabel.c_str(), Vector2( SelectionMarks::labelLeft( row ), row.center().y ), kTextSize, UI_ALIGN_MIDDLE_LEFT,
		mSelected ? MenuPalette::TEXT_SELECTED : MenuPalette::TEXT_DIM, UiMenuItem::kJapaneseSpacing );

	// バー：細い線 → 溜まっている所の明るい線 → 今の値の所のひし形
	int value = mGetValue();
	UiRect bar = barRect();
	float ratio = static_cast< float >( value - mRange.minValue ) / static_cast< float >( mRange.maxValue - mRange.minValue );
	UiRect line = bar;
	line.height = kBarThickness;
	line.top = bar.center().y - kBarThickness * kHalf;
	renderer.addRect( line, MenuPalette::BAR_TRACK );
	line.width = bar.width * ratio;
	renderer.addRect( line, MenuPalette::BAR_FILL );
	renderer.addDiamond( Vector2( bar.left + bar.width * ratio, bar.center().y ), kThumbRadius, MenuPalette::TEXT );

	// 数字は欄の右に寄せる（桁が変わっても右端が揃う）
	std::string text = mFormatValue( value );
	float valueRight = row.right() - kPaddingRight;
	renderer.addText( text.c_str(), Vector2( valueRight - renderer.textWidth( text.c_str(), kTextSize ), row.center().y ), kTextSize, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT );
}

bool UiSlider::contains( const Vector2& point ) const {
	return screenRect().contains( point );
}

void UiSlider::setSelected( bool selected ) {
	mSelected = selected;
}

void UiSlider::activate() {
	// 決定キーでは何もしない（値は左右で変える。行の上でのクリックは UiCanvas が dragTo へ回す）
}

void UiSlider::adjust( int direction ) {
	setClamped( mGetValue() + direction * mRange.step );
}

void UiSlider::dragTo( const Vector2& cursor ) {
	UiRect bar = barRect();
	float ratio = ScalarMath::clamp( ( cursor.x - bar.left ) / bar.width, 0.f, 1.f );
	float value = static_cast< float >( mRange.minValue ) + ratio * static_cast< float >( mRange.maxValue - mRange.minValue );
	setClamped( static_cast< int >( std::lround( value ) ) );
}

UiRect UiSlider::screenRect() const {
	UiRect rect = mRect;
	rect.left += UiLayout::anchorX( mAnchor );
	return rect;
}

UiRect UiSlider::barRect() const {
	UiRect row = screenRect();
	UiRect bar;
	bar.left = SelectionMarks::labelLeft( row ) + kLabelWidth + kColumnGap;
	bar.width = row.right() - kPaddingRight - kValueWidth - kColumnGap - bar.left;
	bar.height = kBarHitHeight;
	bar.top = row.center().y - kBarHitHeight * kHalf;
	return bar;
}

void UiSlider::setClamped( int value ) {
	// 刻みに揃える（ドラッグで 83 になったら 85 へ）。最小から数えた刻みの数を四捨五入する
	float steps = static_cast< float >( value - mRange.minValue ) / static_cast< float >( mRange.step );
	int snapped = mRange.minValue + static_cast< int >( std::lround( steps ) ) * mRange.step;
	mSetValue( std::min( std::max( snapped, mRange.minValue ), mRange.maxValue ) );
}
