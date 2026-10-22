#ifndef INCLUDED_PAUSE_MENU_H
#define INCLUDED_PAUSE_MENU_H

#include "GameSrc/UI/UiScreen.h"
#include <functional>

class UiRenderer;

// 本編の一時停止メニュー（ESC）。止めた本編の画の上に半透明の黒を重ね、見出しとボタン（MenuCatalog::pause）を出す。
// 世界・音を止める・動かし直すのは本編（GameScene）。ここはメニューの操作と描画だけを持つ。
// ボタンの並べ方はメニュー画面と同じ（MenuLayout）。SummaryPlan/個別仕様/PauseMenu_Plan.txt。
class PauseMenu{
public:
	// renderer: 描く道具。所有権は受け取らない。nullptr 不可。このメニューより長く生きていること。
	// onResume: Resume が押されたときの処理（本編が止めた世界を動かし直す）。空は不可。
	PauseMenu( UiRenderer* renderer, std::function< void() > onResume );

	// 入力を読んでボタンを操作する。開いている間、1回の描画につき1回（本編の beginFrame で）。
	void handleInput();
	// 暗くする板とメニューを描く。開いている間、1回の描画につき1回。本編の 3D を描いた後に呼ぶ（上に重ねる）。
	// 中で UiRenderer::draw まで行うので、同じ描画で HUD（GameHud::draw）は描かないこと（UiRenderer は 1 回の描画に 1 回だけ）。
	void draw();

private:
	UiScreen mScreen;
	UiRenderer* mRenderer; // 借りているだけ
};

#endif
