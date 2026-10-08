#ifndef INCLUDED_UI_LABEL_H
#define INCLUDED_UI_LABEL_H

#include "IUiElement.h"
#include "UiRenderer.h"
#include "GameLib/Math/Vector2.h"
#include <string>

// 文字を表示するだけの部品（画面の見出し・HUD の文字）。文字は後から変えられる（HUD の「Fuel 1/3」など）。
class UiLabel : public IUiElement{
public:
	// position / align: 文字を置く位置とその基準（UiAlign）。color: VertexColor::pack の並び。
	UiLabel( const char* text, const GameLib::Math::Vector2& position, UiTextSize size, UiAlign align, unsigned color );

	void setText( const char* text );
	virtual void draw( UiRenderer& renderer ) const;

private:
	std::string mText;
	GameLib::Math::Vector2 mPosition;
	UiTextSize mSize;
	UiAlign mAlign;
	unsigned mColor;
};

#endif
