#ifndef INCLUDED_UI_MENU_ITEM_H
#define INCLUDED_UI_MENU_ITEM_H

#include "GameSrc/UI/IUiElement.h"
#include "GameSrc/UI/Widget/IUiSelectable.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/UI/UiRect.h"
#include "GameSrc/UI/UiRenderer.h"
#include <functional>
#include <string>

// RE:2 風のメニューの 1 行（板は描かない）。選ばれている間は、上下の細い線・帯・赤いひし形を出す（SelectionMarks）
// 文字は左に寄せる（縦に並べるメニュー）か、真ん中に置く（ゲームオーバーの横並び）
// 押したときの処理は作る側が onActivate で渡し、どれを選ぶか・いつ決定するかは UiCanvas が決める
class UiMenuItem : public IUiElement, public IUiSelectable{
public:
	// 字の間（画素）。英字の大文字は広く、日本語は詰めて並べる
	static constexpr float kLatinSpacing = 5.f;
	static constexpr float kJapaneseSpacing = 2.4f;

	// anchor / rect: 行の範囲（画素。マウスで選べる範囲もこれ）。rect.left は anchor からのずれ、rect.top は画面の上端から
	// label: 書く文字。onActivate: 押したときの処理（空は不可）。letterSpacing: 字の間。kLatinSpacing か kJapaneseSpacing
	// labelAlign: 文字の置き方。UI_ALIGN_MIDDLE_LEFT（左に寄せる）か UI_ALIGN_CENTER（真ん中に置く）
	UiMenuItem( UiAnchor anchor, const UiRect& rect, const char* label, std::function< void() > onActivate, float letterSpacing,
		UiAlign labelAlign = UI_ALIGN_MIDDLE_LEFT );

	void draw( UiRenderer& renderer ) const override;
	bool contains( const GameLib::Math::Vector2& point ) const override;
	void setSelected( bool selected ) override;
	void activate() override;

private:
	UiRect screenRect() const;	// 今の画面上の範囲。描くときと、カーソルが乗っているかを調べるときに同じ範囲を使う

	UiAnchor mAnchor;
	UiRect mRect; // left は mAnchor からのずれ（理由は UiAnchor.h）
	std::string mLabel;
	std::function< void() > mOnActivate;
	bool mSelected;
	float mLetterSpacing;
	UiAlign mLabelAlign;
};

#endif
