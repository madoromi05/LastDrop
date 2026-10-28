#ifndef INCLUDED_EDITOR_WINDOW_H
#define INCLUDED_EDITOR_WINDOW_H

// 開発用のエディタの窓（F6 で開く。Debug・Profile ビルドだけ）。計画書は SummaryPlan/個別仕様/DebugTools_Profiling_Plan.txt の 4 章「見た目」。
// 画面の右端に上から下まで 1 枚出し、見出しで欄を開け閉めする。画面は暗くせず、ゲームも止めない。
//   Scene ：一時停止・1 ティック進める・時間の倍率（SceneManager の時計。ESC の一時停止とは別で、メニューも音も触らない）
//   Render：VSync の切り替え（CPU と GPU のどちらが重いかを見分けるときに切る）と、描き方（ディファード / フォワード）・G-Buffer の中身の表示（DeferredLighting）
//   Perf  ：計測の数字・グラフ・区間・印・ロード・GPU（中身は GameLib の Profiling::drawPerfPanel）
//   Tuning：調整値の構造体（TuningPanel。自キャラ・銃・視点・ゾンビの群れ。プリセットの保存・読み込みと Copy as C++）
//   Zombies / Audio：まだ灰色の見出しだけ
// ImGui の土台（開け閉め・入力の受け渡し）は GameLib の DebugUi が持つ。ここは窓の中身だけ。
namespace EditorWindow{

// エディタが開いているときに、1 回の描画につき 1 回呼ぶ（SceneManager::update の後）。Release では何もしない。
void draw();
// 自キャラが無敵か（F5）を、窓の一番上に出すために受け取る。窓は自キャラを知らないので、GameScene から渡す。
// GameScene が終わるときに false へ戻す（タイトルなどで ON のまま残さない）。Release では何もしない。
void setPlayerInvincible( bool invincible );

} //namespace EditorWindow

#endif
