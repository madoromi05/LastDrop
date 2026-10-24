#ifndef INCLUDED_EDITOR_WINDOW_H
#define INCLUDED_EDITOR_WINDOW_H

// 開発用のエディタの窓（F6 で開く。Debug・Profile ビルドだけ）。計画書は SummaryPlan/個別仕様/DebugTools_Profiling_Plan.txt の 4 章「見た目」。
// 画面の右端に上から下まで 1 枚出し、見出しで欄を開け閉めする。画面は暗くせず、ゲームも止めない。
//   Scene ：一時停止・1 ティック進める・時間の倍率（SceneManager の時計。ESC の一時停止とは別で、メニューも音も触らない）
//   Render：VSync の切り替え（CPU と GPU のどちらが重いかを見分けるときに切る）
//   Perf  ：計測の数字・グラフ・区間・印・ロード・GPU（中身は GameLib の Profiling::drawPerfPanel）
//   Player / Camera / Weapon、Zombies / Audio：まだ灰色の見出しだけ（調整値の構造体を作る手順 5 で中身を入れる）
// ImGui の土台（開け閉め・入力の受け渡し）は GameLib の DebugUi が持つ。ここは窓の中身だけ。
namespace EditorWindow{

// エディタが開いているときに、1 回の描画につき 1 回呼ぶ（SceneManager::update の後）。Release では何もしない。
void draw();

} //namespace EditorWindow

#endif
