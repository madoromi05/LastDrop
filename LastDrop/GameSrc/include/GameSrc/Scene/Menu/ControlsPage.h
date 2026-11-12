#ifndef INCLUDED_CONTROLS_PAGE_H
#define INCLUDED_CONTROLS_PAGE_H

#include <functional>

class UiCanvas;

// 操作の一覧のページ（左上の見出し「操作の一覧」・「移動」「戦う」「そのほか」のまとまりを 3 列・「戻る」の行・下の操作の案内）
// キーは今の割り当て（Pad::bindings）から作るので、割り当てを変えても一覧が合う。キーの名前は GameLib が持つ
// （GameLib::Input::BindingDisplayName）。ここが持つのは「どの操作を、どの名前で、どの順に見せるか」だけ
// 遊ぶ操作だけを見せる（開発用の F キーと、今は何も起こらない Jump / Dodge は出さない）
// 見た目はタイトルと同じ RE:2 風（MenuLayout::buildSideHeading・UiKeyBindingGroup・UiMenuItem）。MenuRestyle_Plan.txt
namespace ControlsPage{
	// canvas に部品を足す。canvas: nullptr 不可。back: 「戻る」が押されたときに呼ぶ。空は不可
	void build( UiCanvas* canvas, std::function< void() > back );
}

#endif
