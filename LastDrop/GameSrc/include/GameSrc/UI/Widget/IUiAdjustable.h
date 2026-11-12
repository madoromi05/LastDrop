#ifndef INCLUDED_I_UI_ADJUSTABLE_H
#define INCLUDED_I_UI_ADJUSTABLE_H

#include "GameLib/Math/Vector2.h"

// 値を左右に動かせる部品（音量のスライダーなど）。選ぶ・決定するのは IUiSelectable と同じで、これは「値の変え方」だけを足す
// いつ変えるか（左右キー・ドラッグ）は UiCanvas が決め、部品は値をどう動かすかだけを持つ
class IUiAdjustable{
public:
	virtual ~IUiAdjustable(){}
	// 1 刻みだけ動かす。direction: -1 で下げる、+1 で上げる
	virtual void adjust( int direction ) = 0;
	// cursor（画素）の横の位置に合う値にする（バーをクリック・ドラッグしたとき）
	virtual void dragTo( const GameLib::Math::Vector2& cursor ) = 0;
};

#endif
