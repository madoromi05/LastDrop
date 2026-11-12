#ifndef INCLUDED_SELECTION_MARKS_H
#define INCLUDED_SELECTION_MARKS_H

#include "GameSrc/UI/UiRect.h"

class UiRenderer;

// 選ばれている行の印（上下の細い線・左から右へ薄れる帯・赤いひし形）を描く。メニューの行（UiMenuItem）と設定の行（UiSlider）が使う
// 色は MenuPalette。MenuRestyle_Plan.txt
namespace SelectionMarks{
	// row の範囲に印を積む
	void draw( UiRenderer& renderer, const UiRect& row );
	// 印の右で、行の文字を書き始める x。選ばれていない行も同じ所に揃える
	float labelLeft( const UiRect& row );
}

#endif
