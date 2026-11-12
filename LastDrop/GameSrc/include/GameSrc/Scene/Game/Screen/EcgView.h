#ifndef INCLUDED_ECG_VIEW_H
#define INCLUDED_ECG_VIEW_H

#include "GameSrc/UI/UiRect.h"

class UiRenderer;
enum class PlayerCondition;

// 体力の状態を心電図（ECG）の波線で見せる（RE:2 風）。インベントリ画面の左下に 1 つ
// 線を引く命令が無いので、UiRenderer::addRect（細い長方形）を横に並べて折れ線にする
// （ItemGridView::addFrame と同じ考え方）。状態は持たない（渡された condition を描くだけ）
// UiRenderer::draw は呼ばない（積むだけ。使う側＝InventoryScreen が最後に 1 回）
class EcgView{
public:
	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この部品より長く生きていること
	explicit EcgView( UiRenderer* renderer );

	// area の中に、condition の色・波形で波線を積む。開いている間、1 回の描画につき 1 回
	void draw( const UiRect& area, PlayerCondition condition );

private:
	UiRenderer* mRenderer; // 借りているだけ
};

#endif