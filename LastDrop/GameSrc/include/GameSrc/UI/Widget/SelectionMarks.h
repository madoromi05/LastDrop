#ifndef INCLUDED_SELECTION_MARKS_H
#define INCLUDED_SELECTION_MARKS_H

#include "GameSrc/UI/UiRect.h"

class UiRenderer;

// 選ばれている行の印（上下の細い線・帯・赤いひし形）を描く。メニューの行（UiMenuItem）と設定の行（UiSlider）が使う
// 文字を左に寄せる行は、帯を左から右へ薄れさせ、ひし形を左端に置く
// 文字を真ん中に置く行（ゲームオーバーの横並び）は、帯を行いっぱいに塗り、ひし形を文字のすぐ左に置く
// 色は MenuPalette。MenuRestyle_Plan.txt
namespace SelectionMarks{
	void draw( UiRenderer& renderer, const UiRect& row );	// 文字を左に寄せる行。row の範囲に印を積む
	float labelLeft( const UiRect& row );					// 文字を左に寄せる行で、印の右の、文字を書き始める x。選ばれていない行も同じ所に揃える
	// 文字を真ん中に置く行。row の範囲に印を積む。labelWidth: 行の文字の幅
	void drawCentered( UiRenderer& renderer, const UiRect& row, float labelWidth );
	// 文字を真ん中に置く行で、文字を書き始める x。ひし形・間・文字をまとめて行の真ん中に置く
	// 選ばれていない行も同じ所に揃える（選び直しても文字が動かないように）
	float centeredLabelLeft( const UiRect& row, float labelWidth );
}

#endif
