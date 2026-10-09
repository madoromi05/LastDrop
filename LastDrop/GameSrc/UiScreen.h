#ifndef INCLUDED_UI_SCREEN_H
#define INCLUDED_UI_SCREEN_H

#include "UiCanvas.h"
#include "UiRenderer.h"
#include "UiRect.h"
#include "GameLib/Math/Vector2.h"

// 部品を並べた1画面（タイトル・クリア画面・本編の HUD）。部品の持ち主（UiCanvas）・描く道具（UiRenderer）・
// 入力の読み取りを1つにまとめ、シーンは「何を並べるか」だけを書けばよいようにする（シーンが持つ部品として使う）。
// 並べる位置の決まり（見出しの位置・ボタンの縦の並び）もここに置き、画面ごとに数字を書かないようにする。
class UiScreen{
public:
	UiScreen();

	UiCanvas& canvas() { return mCanvas; }

	// 入力を読んで部品を操作する。Pad::update の後、1回の描画につき1回（シーンの beginFrame で）呼ぶ。
	void handleInput();
	// 部品を画面に描く。1回の描画につき1回。3D を描く画面では 3D の後に呼ぶ（上に重ねるため）。
	void draw();

	// ── 並べる位置（画素）──
	// 画面の見出しの位置（中央揃えで使う）。
	static GameLib::Math::Vector2 headingPosition();
	// 画面中央に縦に並べるボタンの index 番目（0 が一番上）の範囲。
	static UiRect menuButtonRect( int index );
	// 画面の中央（照準）。
	static GameLib::Math::Vector2 screenCenter();
	// 見ている物の案内を出す位置（照準の少し下。中央揃えで使う）。
	static GameLib::Math::Vector2 promptPosition();
	// 画面の左下に並べる状態表示の index 行目（0 が一番下）の左上（左揃えで使う）。
	static GameLib::Math::Vector2 statusLinePosition( int index );

private:
	UiCanvas mCanvas;
	UiRenderer mRenderer;
	GameLib::Math::Vector2 mPreviousCursor; // 前回のカーソルの位置（動いたかどうかを決めるため）
};

#endif
