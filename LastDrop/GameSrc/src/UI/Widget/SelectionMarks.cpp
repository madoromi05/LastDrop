#include "GameSrc/UI/Widget/SelectionMarks.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/MenuPalette.h"

using namespace GameLib::Math;

namespace{
	const float kPaddingX = 11.f;		// 行の中の並び（画素。座標の高さ 480 で）。左から 余白 → ひし形 → 間 → 文字
	const float kMarkRadius = 3.5f;		// ひし形の中心から角まで
	const float kMarkDiameter = kMarkRadius * 2.f;
	const float kMarkToLabel = 12.f;	// ひし形の右の角から文字まで
	const float kLineThickness = 1.f;	// 上下の細い線の太さ
	const float kHalf = 0.5f;

	// 上下の細い線
	void drawLines( UiRenderer& renderer, const UiRect& row ) {
		UiRect line = row;
		line.height = kLineThickness;
		renderer.addRect( line, MenuPalette::SELECTED_LINE ); // 上の線
		line.top = row.bottom() - kLineThickness;
		renderer.addRect( line, MenuPalette::SELECTED_LINE ); // 下の線
	}

	// 文字を真ん中に置く行で、ひし形の左の角の x
	float centeredMarkLeft( const UiRect& row, float labelWidth ) {
		return row.center().x - ( kMarkDiameter + kMarkToLabel + labelWidth ) * kHalf;
	}
}

void SelectionMarks::draw( UiRenderer& renderer, const UiRect& row ) {
	renderer.addGradientRect( row, MenuPalette::SELECTED_BAND, MenuPalette::SELECTED_BAND_END );
	drawLines( renderer, row );
	renderer.addDiamond( Vector2( row.left + kPaddingX + kMarkRadius, row.center().y ), kMarkRadius, MenuPalette::ACCENT );
}

float SelectionMarks::labelLeft( const UiRect& row ) {
	return row.left + kPaddingX + kMarkDiameter + kMarkToLabel;
}

void SelectionMarks::drawCentered( UiRenderer& renderer, const UiRect& row, float labelWidth ) {
	renderer.addRect( row, MenuPalette::SELECTED_BAND );
	drawLines( renderer, row );
	renderer.addDiamond( Vector2( centeredMarkLeft( row, labelWidth ) + kMarkRadius, row.center().y ), kMarkRadius, MenuPalette::ACCENT );
}

float SelectionMarks::centeredLabelLeft( const UiRect& row, float labelWidth ) {
	return centeredMarkLeft( row, labelWidth ) + kMarkDiameter + kMarkToLabel;
}
