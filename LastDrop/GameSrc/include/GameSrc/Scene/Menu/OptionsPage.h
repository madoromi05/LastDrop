#ifndef INCLUDED_OPTIONS_PAGE_H
#define INCLUDED_OPTIONS_PAGE_H

#include "GameSrc/Scene/Menu/MenuLayout.h"
#include <functional>

class UiCanvas;
class AudioEngine;
struct GameSettings;

// 設定のページ（見出し「OPTIONS」・音量 2 つと感度のスライダー・Controls と Back のボタン）。
// 何を並べるか（文言・範囲・並び）はここだけに書く（MenuCatalog と同じ考え方）。ページの切り替えは MenuPages。
// SummaryPlan/個別仕様/UI/OptionsMenu_Plan.txt の 2 章。
namespace OptionsPage{
	// canvas に部品を足す。canvas / settings / audio: 借りる。nullptr 不可。canvas より長く生きていること。
	// スライダーを動かすと settings の値を書き換え、音量はすぐ audio へ流す（保存は MenuPages がページを閉じるとき）。
	// openPage: Controls が押されたときに呼ぶ。back: Back が押されたときに呼ぶ。どちらも空は不可。
	void build( UiCanvas* canvas, GameSettings* settings, AudioEngine* audio,
		std::function< void( MenuPageId ) > openPage, std::function< void() > back );
}

#endif
