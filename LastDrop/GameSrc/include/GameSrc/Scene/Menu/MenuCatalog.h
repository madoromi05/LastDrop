#ifndef INCLUDED_MENU_CATALOG_H
#define INCLUDED_MENU_CATALOG_H

#include "GameSrc/Scene/Menu/MenuScene.h"

// メニュー画面の中身の一覧。画面ごとの文言・色・ボタンの行き先は、ここ（MenuCatalog.cpp）だけに書く。
// 画面を足すときは、ここに関数を1つ足し、SceneManager の SceneId と生成分岐に1行ずつ足す。
namespace MenuCatalog{
	// タイトル画面。ゲーム名と、Start（本編へ）・Quit（終了）。
	MenuDefinition title();
	// クリア画面（ガソリンを車へ運び終えた後）。見出しと、Retry（本編をもう一度）・Title（タイトルへ）。
	MenuDefinition clear();
	// ゲームオーバー画面（自キャラが死んで暗くなった後）。見出しと、Retry（本編をもう一度）・Title（タイトルへ）。
	MenuDefinition gameOver();
}

#endif
