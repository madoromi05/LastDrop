#ifndef INCLUDED_UI_SHADE_H
#define INCLUDED_UI_SHADE_H

#include "GameSrc/UI/IUiElement.h"

// 画面の左を暗くする影。左端から solidUntil までは color のまま塗り、そこから fadeUntil までで透明へ薄れる
// 背景の絵の上、文字の下に置いて文字を読みやすくする（タイトル画面。一時停止でも使う予定なので背景の絵とは分ける）
// 位置は画面の幅に対する割合なので、ウィンドウの横幅が変わっても同じ見え方になる
class UiShade : public IUiElement{
public:
	// color: 影の色（VertexColor::pack の並び。a が一番濃い所の濃さ）
	// solidUntil / fadeUntil: 画面の幅に対する割合（0..1）。0 <= solidUntil <= fadeUntil <= 1
	UiShade( unsigned color, float solidUntil, float fadeUntil );

	void draw( UiRenderer& renderer ) const override;

private:
	unsigned mColor;
	float mSolidUntil;
	float mFadeUntil;
};

#endif
