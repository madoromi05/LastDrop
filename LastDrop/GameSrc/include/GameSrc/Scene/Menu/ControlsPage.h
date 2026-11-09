#ifndef INCLUDED_CONTROLS_PAGE_H
#define INCLUDED_CONTROLS_PAGE_H

#include <functional>

class UiCanvas;

// 操作の一覧のページ（見出し「CONTROLS」・操作の名前とキーの 2 列・Back のボタン）。
// キーは今の割り当て（Pad::bindings）から作るので、割り当てを変えても一覧が合う。キーの名前は GameLib が持つ
// （GameLib::Input::BindingDisplayName）。ここが持つのは「どの操作を、どの名前で、どの順に見せるか」だけ。
// 遊ぶ操作だけを見せる（開発用の F キーと、今は何も起こらない Jump / Dodge は出さない）。
// SummaryPlan/個別仕様/UI/OptionsMenu_Plan.txt の 2 章。
namespace ControlsPage{
	// canvas に部品を足す。canvas: nullptr 不可。back: Back が押されたときに呼ぶ。空は不可。
	void build( UiCanvas* canvas, std::function< void() > back );
}

#endif
