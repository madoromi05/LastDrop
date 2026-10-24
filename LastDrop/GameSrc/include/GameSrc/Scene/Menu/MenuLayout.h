#ifndef INCLUDED_MENU_LAYOUT_H
#define INCLUDED_MENU_LAYOUT_H

#include <functional>
#include <vector>

class UiCanvas;

// メニューの中で開くページ（タイトル・一時停止のボタンから開く設定画面など）。切り替えるのは MenuPages。
enum MenuPageId{
	MENU_PAGE_NONE,     // ページを開かない（ボタンの onActivate を呼ぶ）
	MENU_PAGE_MAIN,     // 最初のページ（MenuDefinition の見出しとボタン）
	MENU_PAGE_OPTIONS,  // 設定（音量・感度）。OptionsPage
	MENU_PAGE_CONTROLS, // 操作の一覧。ControlsPage
};

// メニュー1枚の中身（見出しと、縦に並べるボタン）。何を並べるかだけを書いたデータで、作るのは MenuCatalog。
struct MenuDefinition{
	// ボタン1つ。
	struct Button{
		const char* label;                  // ボタンに書く文字（英語）
		std::function< void() > onActivate; // 決定されたときの処理（シーンの切り替えの予約・終了など）。opens が MENU_PAGE_NONE なら空は不可
		MenuPageId opens = MENU_PAGE_NONE;  // 決定されたら開くページ（設定画面など）。開くボタンでは onActivate は空でよい（使わない）
	};
	const char* heading;            // 画面の見出し（英語）
	unsigned headingColor;          // 見出しの色（VertexColor::pack の並び）
	std::vector< Button > buttons;  // 上から並べる順。最初のボタンが、始めに選ばれている
};

// メニューの並べ方（見出しを上に、ボタンを画面の中央から下へ縦に）。メニュー画面（MenuScene）と本編の一時停止（PauseMenu）で共有し、
// 並べ方を 1 か所にする。
namespace MenuLayout{
	// definition の見出しとボタンを canvas に足す。canvas: nullptr 不可。
	// openPage: ページを開くボタン（opens が MENU_PAGE_NONE 以外）が押されたときに呼ぶ。そのボタンがあるなら空は不可。
	void build( UiCanvas* canvas, const MenuDefinition& definition, std::function< void( MenuPageId ) > openPage );
}

#endif
