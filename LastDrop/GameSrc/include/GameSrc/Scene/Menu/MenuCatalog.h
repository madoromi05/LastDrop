#ifndef INCLUDED_MENU_CATALOG_H
#define INCLUDED_MENU_CATALOG_H

#include "GameSrc/Scene/Menu/MenuLayout.h"

// メニューの中身の一覧。画面ごとの文言・色・ボタンの行き先は、ここ（MenuCatalog.cpp）だけに書く。
// 画面を足すときは、ここに関数を1つ足し、SceneManager の SceneId と生成分岐に1行ずつ足す。
namespace MenuCatalog{
	// タイトル画面。ゲーム名と、Start（拠点へ）・Options（設定）・Quit（終了）。
	// クリア・ゲームオーバーの画面は、スコアを出すので RunEndScene が持つ（2026-11-05。R2_Plan.txt Q5）。
	MenuDefinition title();
	// 本編の一時停止（ESC。シーンではなく本編の上に重ねる）。見出しと、Resume（続ける）・Options・Retry day（その日のやり直し）・Title・Quit。
	// onResume: Resume が押されたときの処理（本編が渡す。止めた世界を動かし直す）。空は不可。
	MenuDefinition pause( std::function< void() > onResume );
}

#endif
