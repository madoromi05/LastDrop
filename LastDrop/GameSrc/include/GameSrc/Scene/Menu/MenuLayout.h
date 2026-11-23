#ifndef INCLUDED_MENU_LAYOUT_H
#define INCLUDED_MENU_LAYOUT_H

#include "GameSrc/Audio/SoundCatalog.h"
#include <functional>
#include <vector>

class UiCanvas;

// メニューの中で開くページ（タイトル・一時停止のボタンから開く設定画面など）。切り替えるのは MenuPages
enum MenuPageId{
	MENU_PAGE_NONE,     // ページを開かない（ボタンの onActivate を呼ぶ）
	MENU_PAGE_MAIN,     // 最初のページ（MenuDefinition の見出しとボタン）
	MENU_PAGE_OPTIONS,  // 設定（音声・感度・操作のタブ）。OptionsPage
};

// メニュー1枚の中身（見出しと、縦に並べるボタン）。何を並べるかだけを書いたデータで、作るのは MenuCatalog
struct MenuDefinition{
	// ボタン1つ
	struct Button{
		const char* label;                  // ボタンに書く文字（英語の大文字。字の間を広げて並べるので）
		std::function< void() > onActivate; // 決定されたときの処理（シーンの切り替えの予約・終了など）。opens が MENU_PAGE_NONE なら空は不可
		MenuPageId opens = MENU_PAGE_NONE;  // 決定されたら開くページ（設定画面など）。開くボタンでは onActivate は空でよい（使わない）
	};
	const char* heading;            // 画面の見出し（英語）
	std::vector< Button > buttons;  // 上から並べる順。最初のボタンが、始めに選ばれている
	// ここから下は、無ければ nullptr
	const char* tagline = nullptr;  // 題名の下のひと言（日本語でよい）
	const char* backdrop = nullptr; // 背景の絵のファイル（幅と高さが 2 の累乗の TGA。UiBackdrop）
	float backdropAlignX = 0.5f;    // 背景の絵の横がはみ出たとき、どこを合わせるか（UiBackdrop の alignX。絵ごとに主役の位置が違う）
	SoundId music = SOUND_NUM;      // 画面にいる間ループで鳴らす曲（MenuScene が MusicPlayer へ渡す）。SOUND_NUM なら曲を止める。一時停止の画面は使わない
};

// メニューの並べ方。RE:2 風で、題名・ひと言・板なしの行を左に寄せる。背景の絵があれば、その左を暗くする
// メニュー画面（MenuScene）と本編の一時停止（PauseMenu）で共有し、並べ方を 1 か所にする
namespace MenuLayout{
	// definition の見出しとボタンを canvas に足す。canvas: nullptr 不可
	// openPage: ページを開くボタン（opens が MENU_PAGE_NONE 以外）が押されたときに呼ぶ。そのボタンがあるなら空は不可
	void build( UiCanvas* canvas, const MenuDefinition& definition, std::function< void( MenuPageId ) > openPage );
	void buildSideHeading( UiCanvas* canvas, const char* heading );	// 左上に見出しと赤い線を足す（設定のページ。最初のページと同じ見た目）。canvas: nullptr 不可。heading: 書く文字
}

#endif
