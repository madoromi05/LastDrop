#ifndef INCLUDED_UI_INPUT_H
#define INCLUDED_UI_INPUT_H

#include "GameLib/Math/Vector2.h"

// 画面の部品（UiCanvas）が使う、1回の描画ぶんの入力の写し。
// 部品は Pad やマウスを直接読まず、これだけを受け取る（入力の出どころが変わっても部品は変わらない。
// 自キャラの PlayerCommand と同じ考え方）。
struct UiInput{
	GameLib::Math::Vector2 cursor; // マウスカーソルの位置（画素。左上が原点）
	bool cursorMoved;              // 前回からカーソルが動いたか（動いたときだけ、カーソルの下の部品を選び直す）
	bool clicked;                  // 左クリックした瞬間
	bool upPressed;                // 選択を上へ（押した瞬間）
	bool downPressed;              // 選択を下へ（押した瞬間）
	bool decidePressed;            // 決定（押した瞬間）

	// 今の入力を読む。Pad::update の後、1回の描画につき1回呼ぶ（押した瞬間は1回しか拾えないため）。
	// previousCursor: 前回読んだときのカーソルの位置（cursorMoved を決めるため。呼ぶ側が持つ）。
	static UiInput read( const GameLib::Math::Vector2& previousCursor );
};

#endif
