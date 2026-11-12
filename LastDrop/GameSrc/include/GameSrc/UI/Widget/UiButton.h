#ifndef INCLUDED_UI_BUTTON_H
#define INCLUDED_UI_BUTTON_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/Widget/IUiSelectable.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRect.h"
#include <functional>
#include <string>

// 押せるボタン。四角い板と、その中央の文字を描く。選ばれている間は板を明るくする
// 押したときに何をするか（シーンの切り替え・終了など）は、作る側が onActivate で渡す
// ボタン自身はシーンのことを知らないので、どの画面でも同じクラスを使える
// どれが選ばれているか・いつ決定されたかは UiCanvas が決める
class UiButton : public IUiElement, public IUiSelectable{
public:
	// anchor / rect: ボタンの範囲（画素）。rect.left は anchor からのずれ、rect.top は画面の上端から
	// label: 中央に書く文字。onActivate: 押したときの処理（空は不可）
	UiButton( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate );

	virtual void draw( UiRenderer& renderer ) const;
	virtual bool contains( const GameLib::Math::Vector2& point ) const;
	virtual void setSelected( bool selected );
	virtual void activate();

private:
	// 今の画面上の範囲。描くときと、カーソルが乗っているかを調べるときに同じ範囲を使う
	UiRect screenRect() const;

	UiAnchor mAnchor;
	UiRect mRect; // left は mAnchor からのずれ。画面上の範囲は screenRect で求める（理由は UiAnchor.h）
	std::string mLabel;
	std::function< void() > mOnActivate;
	bool mSelected;
};

#endif
