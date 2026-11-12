#ifndef INCLUDED_UI_MENU_ITEM_H
#define INCLUDED_UI_MENU_ITEM_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/Widget/IUiSelectable.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRect.h"
#include <functional>
#include <string>

// RE:2 風のメニューの 1 行（左寄せの文字。板は描かない）。選ばれている間は、上下の細い線・左から右へ薄れる帯・赤いひし形を出す
// UiButton（板のボタン）と同じく、押したときの処理は作る側が onActivate で渡し、どれを選ぶか・いつ決定するかは UiCanvas が決める
class UiMenuItem : public IUiElement, public IUiSelectable{
public:
	// 字の間（画素）。英字の大文字は広く、日本語は詰めて並べる
	static constexpr float kLatinSpacing = 5.f;
	static constexpr float kJapaneseSpacing = 2.4f;

	// anchor / rect: 行の範囲（画素。マウスで選べる範囲もこれ）。rect.left は anchor からのずれ、rect.top は画面の上端から
	// label: 書く文字。onActivate: 押したときの処理（空は不可）。letterSpacing: 字の間。kLatinSpacing か kJapaneseSpacing
	UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate, float letterSpacing );

	virtual void draw( UiRenderer& renderer ) const;
	virtual bool contains( const GameLib::Math::Vector2& point ) const;
	virtual void setSelected( bool selected );
	virtual void activate();

private:
	UiRect screenRect() const;	// 今の画面上の範囲。描くときと、カーソルが乗っているかを調べるときに同じ範囲を使う

	UiAnchor mAnchor;
	UiRect mRect; // left は mAnchor からのずれ（理由は UiAnchor.h）
	std::string mLabel;
	std::function< void() > mOnActivate;
	bool mSelected;
	float mLetterSpacing;
};

#endif
