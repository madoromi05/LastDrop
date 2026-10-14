#ifndef INCLUDED_UI_LABEL_H
#define INCLUDED_UI_LABEL_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameLib/Math/Vector2.h"
#include <string>

// 文字を表示するだけの部品（画面の見出し・HUD の文字）。文字は後から変えられる（HUD の「Fuel 1/3」など）。
class UiLabel : public IUiElement{
public:
	// anchor / offset: 文字を置く位置。x は anchor からのずれ、y は画面の上端から（画素）。
	// align: offset が文字のどこを指すか（UiAlign）。color: VertexColor::pack の並び。
	UiLabel( const char* text, UiAnchor anchor, const GameLib::Math::Vector2& offset, UiTextSize size, UiAlign align, unsigned color );

	void setText( const char* text );
	virtual void draw( UiRenderer& renderer ) const;

private:
	std::string mText;
	UiAnchor mAnchor;
	GameLib::Math::Vector2 mOffset; // x は mAnchor からのずれ。画面上の位置は描くときに求める（理由は UiAnchor.h）
	UiTextSize mSize;
	UiAlign mAlign;
	unsigned mColor;
};

#endif
