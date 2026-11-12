#ifndef INCLUDED_I_UI_SELECTABLE_H
#define INCLUDED_I_UI_SELECTABLE_H

#include "GameLib/Math/Vector2.h"

// 選んで決定できる部品（ボタンなど）。どれを選んでいるか・いつ決定するかは UiCanvas が決め、
// 部品は「選ばれている見た目」と「決定されたときの処理」だけを持つ
class IUiSelectable{
public:
	virtual ~IUiSelectable(){}
	// point（画素）が部品の範囲に入っているか（マウスで選ぶため）
	virtual bool contains( const GameLib::Math::Vector2& point ) const = 0;
	// 選ばれている／いないを切り替える（見た目を変えるため）
	virtual void setSelected( bool selected ) = 0;
	// 決定された
	virtual void activate() = 0;
};

#endif
