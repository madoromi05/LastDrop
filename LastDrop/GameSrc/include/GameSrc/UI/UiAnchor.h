#ifndef INCLUDED_UI_ANCHOR_H
#define INCLUDED_UI_ANCHOR_H

#include "GameSrc/UI/UiRect.h"
#include "Framework.h"

// 部品の横の位置を、画面のどこから測るか
// 画面の幅はウィンドウの縦横比に合わせて変わる（高さは変わらない）ので、部品は画素の位置そのものではなく
// 「基準からのずれ」を持ち、描くたびに今の幅から位置を求める。作ったときの幅で位置を決めてしまうと、
// ウィンドウを広げたときに中央の物（照準・見出し・ボタン）が左へずれる
// 縦は高さが変わらないので基準を持たない（上端からの画素のまま）
enum UiAnchor{
	UI_ANCHOR_LEFT,   // 画面の左端から測る
	UI_ANCHOR_CENTER, // 画面の横の中央から測る
};

// 横列
// 画面の左右に余白を空け、その間を等分した列の 1 つ（操作の一覧のまとまり）
// 列は幅も画面の幅で変わるので、UiAnchor と同じく、部品はこれを持って描くたびに位置と幅を求める
struct UiColumn {
	int index;    // 左から何列目か（0 が左端）
	int count;    // 列の数
	float margin; // 画面の左右の端から列までの余白（画素）
	float gap;    // 列と列の間（画素）
};

namespace UiLayout {
	// anchor の今の x（画素）。部品の「基準からのずれ」に足すと画面上の位置になる
	inline float anchorX( UiAnchor anchor ) {
		const float kHalf = 0.5f;
		return ( anchor == UI_ANCHOR_CENTER ) ? GameLib::Framework::instance().width() * kHalf : 0.f;
	}
	// column の今の幅（画素）
	inline float columnWidth( const UiColumn& column ) {
		const float kBothSides = 2.f;											// 余白は左右の 2 か所
		const float inner = GameLib::Framework::instance().width() - column.margin * kBothSides;
		return ( inner - column.gap * ( column.count - 1 ) ) / column.count;	// 間は列と列のあいだにだけあるので、列の数より 1 つ少ない
	}
	// column の今の左端の x（画素）
	inline float columnLeft( const UiColumn& column ) {
		return column.margin + column.index * ( columnWidth( column ) + column.gap );
	}
	// 今の画面全体の範囲（画素）。後ろを暗くする板に使う
	inline UiRect screenRect() {
		UiRect screen;
		screen.left = 0.f;
		screen.top = 0.f;
		screen.width = static_cast< float >( GameLib::Framework::instance().width() );
		screen.height = static_cast< float >( GameLib::Framework::instance().height() );
		return screen;
	}
}

#endif
