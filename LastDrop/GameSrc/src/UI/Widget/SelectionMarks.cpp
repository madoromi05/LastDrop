#include "GameSrc/UI/Widget/SelectionMarks.h"
#include "GameSrc/UI/UiRenderer.h"   // addGradientRect などを呼ぶため
#include "GameSrc/UI/MenuPalette.h"  // 色を使うため

using namespace GameLib::Math;

namespace {
	// 行の中の並び（画素。座標の高さ 480 で）。左から 余白 → ひし形 → 間 → 文字。
	const float kPaddingX = 11.f;
	const float kMarkRadius = 3.5f;        // ひし形の中心から角まで
	const float kMarkDiameter = kMarkRadius * 2.f;
	const float kMarkToLabel = 12.f;       // ひし形の右の角から文字まで
	const float kLineThickness = 1.f;      // 上下の細い線の太さ
}

	void SelectionMarks::draw( UiRenderer& renderer, const UiRect& row ) {
		renderer.addGradientRect( row, MenuPalette::SELECTED_BAND, MenuPalette::SELECTED_BAND_END );
		UiRect line = row;
		line.height = kLineThickness;
		renderer.addRect( line, MenuPalette::SELECTED_LINE ); // 上の線
		line.top = row.bottom() - kLineThickness;
		renderer.addRect( line, MenuPalette::SELECTED_LINE ); // 下の線
		renderer.addDiamond( Vector2( row.left + kPaddingX + kMarkRadius, row.center().y ), kMarkRadius, MenuPalette::ACCENT );
	}

	float SelectionMarks::labelLeft( const UiRect& row ) {
		return row.left + kPaddingX + kMarkDiameter + kMarkToLabel;
	}