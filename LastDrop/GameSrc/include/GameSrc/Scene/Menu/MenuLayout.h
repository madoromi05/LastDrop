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

// メニューの最初のページの並べ方（見た目）。TitleScreen_Plan.txt。
enum MenuLook{
	MENU_LOOK_CENTER, // 見出しを上に、板のボタンを画面の中央に縦に
	MENU_LOOK_SIDE,   // RE:2 風。題名・ひと言・板なしの行を左に寄せる。背景の絵があれば、その左を暗くする（タイトル・一時停止）
};

// メニュー1枚の中身（見出しと、縦に並べるボタン）。何を並べるかだけを書いたデータで、作るのは MenuCatalog。
struct MenuDefinition{
	// ボタン1つ。
	struct Button{
		const char* label;                  // ボタンに書く文字（英語）。MENU_LOOK_SIDE では大文字（字の間を広げて並べるので）
		std::function< void() > onActivate; // 決定されたときの処理（シーンの切り替えの予約・終了など）。opens が MENU_PAGE_NONE なら空は不可
		MenuPageId opens = MENU_PAGE_NONE;  // 決定されたら開くページ（設定画面など）。開くボタンでは onActivate は空でよい（使わない）
	};
	const char* heading;            // 画面の見出し（英語）
	unsigned headingColor;          // 見出しの色（VertexColor::pack の並び）。MENU_LOOK_SIDE では使わない（色は MenuPalette）
	std::vector< Button > buttons;  // 上から並べる順。最初のボタンが、始めに選ばれている
	MenuLook look = MENU_LOOK_CENTER; // 書かなければ中央の並べ方
	// ここから下は MENU_LOOK_SIDE だけで使う。無ければ nullptr。
	const char* tagline = nullptr;  // 題名の下のひと言（日本語でよい）
	const char* backdrop = nullptr; // 背景の絵のファイル（幅と高さが 2 の累乗の TGA。UiBackdrop）
	float backdropAlignX = 0.5f;    // 背景の絵の横がはみ出たとき、どこを合わせるか（UiBackdrop の alignX。絵ごとに主役の位置が違う）
};

// メニューの並べ方（MenuLook ごと）。メニュー画面（MenuScene）と本編の一時停止（PauseMenu）で共有し、
// 並べ方を 1 か所にする。
namespace MenuLayout{
	// definition の見出しとボタンを、definition.look の並べ方で canvas に足す。canvas: nullptr 不可。
	// openPage: ページを開くボタン（opens が MENU_PAGE_NONE 以外）が押されたときに呼ぶ。そのボタンがあるなら空は不可。
	void build( UiCanvas* canvas, const MenuDefinition& definition, std::function< void( MenuPageId ) > openPage );
	// 左上に見出しと赤い線を足す（設定・操作の一覧のページ。MENU_LOOK_SIDE と同じ見た目）。canvas: nullptr 不可。heading: 書く文字。
	void buildSideHeading( UiCanvas* canvas, const char* heading );
}

#endif
