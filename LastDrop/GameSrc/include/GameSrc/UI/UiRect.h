#ifndef INCLUDED_UI_RECT_H
#define INCLUDED_UI_RECT_H

#include "GameLib/Math/Vector2.h"

// 画面上の長方形（画素。左上が原点、右が +x、下が +y）。ボタンの範囲や背景の板に使う。
struct UiRect{
	float left;
	float top;
	float width;
	float height;

	float right() const { return left + width; }
	float bottom() const { return top + height; }
	GameLib::Math::Vector2 center() const {
		const float kHalf = 0.5f;
		return GameLib::Math::Vector2( left + width * kHalf, top + height * kHalf );
	}
	// point が長方形の中（端を含む）にあるか。
	bool contains( const GameLib::Math::Vector2& point ) const {
		return point.x >= left && point.x <= right() && point.y >= top && point.y <= bottom();
	}
};

#endif
