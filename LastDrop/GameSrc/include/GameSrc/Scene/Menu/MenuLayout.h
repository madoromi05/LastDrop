#ifndef INCLUDED_MENU_LAYOUT_H
#define INCLUDED_MENU_LAYOUT_H

#include <functional>
#include <vector>

class UiCanvas;

// メニュー1枚の中身（見出しと、縦に並べるボタン）。何を並べるかだけを書いたデータで、作るのは MenuCatalog。
struct MenuDefinition{
	// ボタン1つ。
	struct Button{
		const char* label;                  // ボタンに書く文字（英語）
		std::function< void() > onActivate; // 決定されたときの処理（シーンの切り替えの予約・終了など）。空は不可
	};
	const char* heading;            // 画面の見出し（英語）
	unsigned headingColor;          // 見出しの色（VertexColor::pack の並び）
	std::vector< Button > buttons;  // 上から並べる順。最初のボタンが、始めに選ばれている
};

// メニューの並べ方（見出しを上に、ボタンを画面の中央から下へ縦に）。メニュー画面（MenuScene）と本編の一時停止（PauseMenu）で共有し、
// 並べ方を 1 か所にする。
namespace MenuLayout{
	// definition の見出しとボタンを canvas に足す。canvas: nullptr 不可。
	void build( UiCanvas* canvas, const MenuDefinition& definition );
}

#endif
