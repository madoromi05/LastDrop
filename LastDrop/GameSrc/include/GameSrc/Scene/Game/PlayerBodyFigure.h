#ifndef INCLUDED_PLAYER_BODY_FIGURE_H
#define INCLUDED_PLAYER_BODY_FIGURE_H

#include "GameSrc/Player/PlayerBodyZone.h"
#include "GameSrc/UI/UiRect.h"

class PlayerHealth;
class UiRenderer;

// 画面の左上に出す、自キャラの体の図（タルコフのように部位ごとの状態を色で見せる。SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 5 章）。
// 7 部位（PlayerBodyZone）を 7 つの四角で人の形に並べ、残りの割合で 緑 → 黄 → 赤、壊れた（0）は暗い灰色に塗る。数字は出さない。
// 正面から見た向き（自分の左腕が画面の右）。画面の左上に、いつも薄く（半透明で）出す。減った部位ほど少し濃くなる（2026-10-21 ユーザー指定）。
// 状態を持たない（毎回の描画で体力を読んで積むだけ）。GameHud が描き、確認用の表示（F3）が部位の横に数字を並べるのに位置を読む。
namespace PlayerBodyFigure{
	// health の今の状態で、7 つの四角を renderer に積む（描くのは UiRenderer::draw）。renderer: nullptr 不可。
	void add( const PlayerHealth& health, UiRenderer* renderer );
	// 図を囲む長方形（画素。左上が原点）。
	UiRect bounds();
}

#endif
