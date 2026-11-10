#ifndef INCLUDED_UI_BAR_H
#define INCLUDED_UI_BAR_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRect.h"

// 塗りつぶした長方形を描くだけの部品（タイトルの題名の下の赤い短い線など、飾りの線）
class UiBar : public IUiElement{
public:
	// anchor / rect: 範囲（画素）。rect.left は anchor からのずれ、rect.top は画面の上端から。color: VertexColor::pack の並び。
	UiBar( UiAnchor anchor, const UiRect& rect, unsigned color );

	virtual void draw( UiRenderer& renderer ) const;

private:
	UiAnchor mAnchor;
	UiRect mRect; // left は mAnchor からのずれ（理由は UiAnchor.h）
	unsigned mColor;
};

#endif
