#ifndef INCLUDED_UI_BUTTON_H
#define INCLUDED_UI_BUTTON_H

#include "IUiElement.h"
#include "IUiSelectable.h"
#include "UiRect.h"
#include <functional>
#include <string>

// 押せるボタン。四角い板と、その中央の文字を描く。選ばれている間は板を明るくする。
// 押したときに何をするか（シーンの切り替え・終了など）は、作る側が onActivate で渡す。
// ボタン自身はシーンのことを知らないので、どの画面でも同じクラスを使える。
// どれが選ばれているか・いつ決定されたかは UiCanvas が決める。
class UiButton : public IUiElement, public IUiSelectable{
public:
	// rect: 画面上の範囲（画素）。label: 中央に書く文字。onActivate: 押したときの処理（空は不可）。
	UiButton( const UiRect& rect, const char* label, std::function< void() > onActivate );

	virtual void draw( UiRenderer& renderer ) const;
	virtual bool contains( const GameLib::Math::Vector2& point ) const;
	virtual void setSelected( bool selected );
	virtual void activate();

private:
	UiRect mRect;
	std::string mLabel;
	std::function< void() > mOnActivate;
	bool mSelected;
};

#endif
