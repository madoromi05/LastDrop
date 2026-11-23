#ifndef INCLUDED_CONTROLS_TAB_H
#define INCLUDED_CONTROLS_TAB_H

class UiCanvas;

// 設定のページの「操作」のタブの中身（上に「キーボード・マウス／パッド」の小さい切り替え・その下に「移動」「戦う」「そのほか」のまとまりを 3 列）
// 切り替えると、まとまりの行だけを差し替える（ページを作り直さないので、選んでいる行が切り替えの行のまま残る）
// キーは今の割り当て（Pad::bindings）から作るので、割り当てを変えても一覧が合う。キーの名前は GameLib が持つ
// （GameLib::Input::BindingDisplayName・パッドは PlayStation の表記の GamePadBindingDisplayName）
// ここが持つのは「どの操作を、どの名前で、どの順に見せるか」だけ
// 遊ぶ操作だけを見せる（開発用の F キーと、今は何も起こらない Jump / Dodge は出さない）
// 見た目はタイトルと同じ RE:2 風（UiTabBar・UiKeyBindingGroup）
namespace ControlsTab{
	void build( UiCanvas* canvas );	// canvas に部品を足す。始めはキーボードとマウスの一覧を見せる。canvas: nullptr 不可
}

#endif
