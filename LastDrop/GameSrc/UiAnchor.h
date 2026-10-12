#ifndef INCLUDED_UI_ANCHOR_H
#define INCLUDED_UI_ANCHOR_H

#include "Framework.h"

// 部品の横の位置を、画面のどこから測るか。
// 画面の幅はウィンドウの縦横比に合わせて変わる（高さは変わらない）ので、部品は画素の位置そのものではなく
// 「基準からのずれ」を持ち、描くたびに今の幅から位置を求める。作ったときの幅で位置を決めてしまうと、
// ウィンドウを広げたときに中央の物（照準・見出し・ボタン）が左へずれる。
// 縦は高さが変わらないので基準を持たない（上端からの画素のまま）。
enum UiAnchor{
	UI_ANCHOR_LEFT,   // 画面の左端から測る
	UI_ANCHOR_CENTER, // 画面の横の中央から測る
};

namespace UiLayout{
	// anchor の今の x（画素）。部品の「基準からのずれ」に足すと画面上の位置になる。
	inline float anchorX( UiAnchor anchor ) {
		const float kHalf = 0.5f;
		return ( anchor == UI_ANCHOR_CENTER ) ? GameLib::Framework::instance().width() * kHalf : 0.f;
	}
}

#endif
