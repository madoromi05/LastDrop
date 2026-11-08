#include "GameSrc/UI/Widget/UiSlider.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include <algorithm>
#include <cmath>

using namespace GameLib::Math;

namespace{
	// 行の板の色。ボタン（UiButton）と同じく、選ばれているかどうかを明るさで分ける。
	const unsigned kPlateColor = VertexColor::pack( 0.15f, 0.15f, 0.15f, 0.8f );
	const unsigned kSelectedPlateColor = VertexColor::pack( 0.3f, 0.3f, 0.3f, 0.95f );
	const unsigned kLabelColor = VertexColor::pack( 0.9f, 0.9f, 0.9f );
	// バー。溜まっている所は、選ばれている間だけボタンと同じ燃料の色（黄）にする。
	const unsigned kBarBackColor = VertexColor::pack( 0.05f, 0.05f, 0.05f, 0.9f );
	const unsigned kBarFillColor = VertexColor::pack( 0.7f, 0.7f, 0.7f );
	const unsigned kSelectedBarFillColor = VertexColor::pack( 0.85f, 0.65f, 0.15f );

	// 行の中の並べ方（画素。行の左端・右端から）。名前 → バー → 数字の順。
	const float kLabelInset = 16.f;     // 行の左端から名前まで
	const float kBarLeftInset = 230.f;  // 行の左端からバーの左端まで（一番長い名前「Mouse Sensitivity」が収まる幅）
	const float kBarRightInset = 90.f;  // 行の右端からバーの右端まで（数字を書く場所を空ける）
	const float kValueRightInset = 70.f; // 行の右端から数字の左端まで
	const float kBarHeight = 12.f;
	const float kHalf = 0.5f;
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
	renderer.addRect( row, mSelected ? kSelectedPlateColor : kPlateColor );
	renderer.addText( mLabel.c_str(), Vector2( row.left + kLabelInset, row.center().y ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, kLabelColor );

	int value = mGetValue();
	UiRect bar = barRect();
	renderer.addRect( bar, kBarBackColor );
	UiRect fill = bar;
	fill.width = bar.width * static_cast< float >( value - mRange.minValue ) / static_cast< float >( mRange.maxValue - mRange.minValue );
	renderer.addRect( fill, mSelected ? kSelectedBarFillColor : kBarFillColor );

	std::string text = mFormatValue( value );
	renderer.addText( text.c_str(), Vector2( row.right() - kValueRightInset, row.center().y ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, kLabelColor );
}

bool UiSlider::contains( const Vector2& point ) const {
	return screenRect().contains( point );
}

void UiSlider::setSelected( bool selected ) {
	mSelected = selected;
}

void UiSlider::activate() {
	// 決定キーでは何もしない（値は左右で変える。行の上でのクリックは UiCanvas が dragTo へ回す）。
}

void UiSlider::adjust( int direction ) {
	setClamped( mGetValue() + direction * mRange.step );
}

void UiSlider::dragTo( const Vector2& cursor ) {
	UiRect bar = barRect();
	float ratio = std::min( std::max( ( cursor.x - bar.left ) / bar.width, 0.f ), 1.f );
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
	bar.left = row.left + kBarLeftInset;
	bar.width = row.width - kBarLeftInset - kBarRightInset;
	bar.height = kBarHeight;
	bar.top = row.center().y - kBarHeight * kHalf;
	return bar;
}

void UiSlider::setClamped( int value ) {
	// 刻みに揃える（ドラッグで 83 になったら 85 へ）。最小から数えた刻みの数を四捨五入する。
	float steps = static_cast< float >( value - mRange.minValue ) / static_cast< float >( mRange.step );
	int snapped = mRange.minValue + static_cast< int >( std::lround( steps ) ) * mRange.step;
	mSetValue( std::min( std::max( snapped, mRange.minValue ), mRange.maxValue ) );
}
