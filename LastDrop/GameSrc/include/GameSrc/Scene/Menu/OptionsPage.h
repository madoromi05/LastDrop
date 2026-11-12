#ifndef INCLUDED_OPTIONS_PAGE_H
#define INCLUDED_OPTIONS_PAGE_H

#include <functional>

class UiCanvas;
class AudioEngine;
struct GameSettings;

// 設定のページのタブ。並びは左からの順
enum OptionsTab{
	OPTIONS_TAB_AUDIO,       // 音声（全体の音量・効果音の音量）
	OPTIONS_TAB_SENSITIVITY, // 感度（マウス・パッド）
	OPTIONS_TAB_CONTROLS,    // 操作の一覧（ControlsTab）

	OPTIONS_TAB_NUMBER,
};

// 設定のページ（左上の見出し「設定」・その下のタブの行・今のタブの中身・「戻る」の行・下の操作の案内）
// タブを変えたらページを作り直す（タブごとに並べる部品が違い、UiCanvas は部品を外せないため。作り直すのは MenuPages）
// 何を並べるか（文言・範囲・並び）はここだけに書く（MenuCatalog と同じ考え方）。操作のタブの中身は ControlsTab
// 見た目はタイトルと同じ RE:2 風（MenuLayout::buildSideHeading・UiTabBar・UiSlider・UiMenuItem）。OptionsTabs_Plan.txt
namespace OptionsPage{
	// canvas に部品を足す。canvas / settings / audio: 借りる。nullptr 不可。canvas より長く生きていること
	// スライダーを動かすと settings の値を書き換え、音量はすぐ audio へ流す（保存は MenuPages がページを閉じるとき）
	// tab: 中身を見せるタブ。selectTab: タブの行で別のタブが選ばれたときに、そのタブで呼ぶ。back: 「戻る」が押されたときに呼ぶ。どちらも空は不可
	void build( UiCanvas* canvas, GameSettings* settings, AudioEngine* audio, OptionsTab tab,
		std::function< void( OptionsTab ) > selectTab, std::function< void() > back );
}

#endif
