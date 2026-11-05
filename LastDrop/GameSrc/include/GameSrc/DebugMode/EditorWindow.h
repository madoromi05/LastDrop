#ifndef INCLUDED_EDITOR_WINDOW_H
#define INCLUDED_EDITOR_WINDOW_H

// 開発用のエディタの窓（F6 で開く。Debug・Profile ビルドだけ）。計画書は SummaryPlan/個別仕様/DebugTools_Profiling_Plan.txt の 4 章「見た目」。
// 画面の右端に上から下まで 1 枚出し、見出しで欄を開け閉めする。画面は暗くせず、ゲームも止めない。
//   Scene ：一時停止・1 ティック進める・時間の倍率（SceneManager の時計。ESC の一時停止とは別で、メニューも音も触らない）
//   Render：VSync の切り替え（CPU と GPU のどちらが重いかを見分けるときに切る）と、昼・夜の光（LightingParams）・影の ON/OFF・G-Buffer と影の中身の表示（DeferredLighting）
//   Perf  ：計測の数字・グラフ・区間・印・ロード・GPU（中身は GameLib の Profiling::drawPerfPanel）
//   Tuning：調整値の構造体（TuningPanel。自キャラ・銃・視点・ゾンビの群れ。プリセットの保存・読み込みと Copy as C++）
//   Objects：世界に置いた物の一覧（Unity の Hierarchy。名前・タグ・部品の数。ComponentSystem2_Plan.txt の ★6）
//   Inventory：銃 2 種と弾 2 種を好きな数だけ足す・減らす・カバンを空にする・行を増やす／減らす（GridInventory_Plan.txt。拾いに行かずに試すため）
//   Raid：残り時間を変える・すぐ脱出する・日を変える・倉庫に物を足す・保存（save.xml）を消す（R2_Plan.txt。5 日間の流れをすぐ試すため）
//   Zombies / Audio：まだ灰色の見出しだけ
// ImGui の土台（開け閉め・入力の受け渡し）は GameLib の DebugUi が持つ。ここは窓の中身だけ。
class GameObjectWorld;
class Inventory;
class RaidSession;
class RunState;
class InteractionSystem;

namespace EditorWindow{

// エディタが開いているときに、1 回の描画につき 1 回呼ぶ（SceneManager::update の後）。Release では何もしない。
void draw();
// 自キャラが無敵か（F5）を、窓の一番上に出すために受け取る。窓は自キャラを知らないので、GameScene から渡す。
// GameScene が終わるときに false へ戻す（タイトルなどで ON のまま残さない）。Release では何もしない。
void setPlayerInvincible( bool invincible );
// Objects の欄に一覧を出す物の持ち主。窓は世界を知らないので、GameScene から渡す。所有権は受け取らない。
// nullptr なら欄に何も出さない。GameScene が終わるときに nullptr へ戻す（消えた世界を指し続けないように）。Release では何もしない。
void setObjects( const GameObjectWorld* objects );
// Inventory の欄で触るカバン。窓は自キャラを知らないので、GameScene から渡す。所有権は受け取らない。
// nullptr なら欄に何も出さない。GameScene が終わるときに nullptr へ戻す。Release では何もしない。
void setInventory( Inventory* inventory );
// Raid の欄で触る、町の 1 日（残り時間・すぐ脱出）と 5 日間の様子（日・倉庫に物を足す・保存を消す）。R2_Plan.txt 9 章。
// どちらも窓は知らないので、シーンが渡す（RaidSession は本編、RunState は本編と拠点）。所有権は受け取らない。
// nullptr ならその行を出さない。シーンが終わるときに nullptr へ戻す。Release では何もしない。
void setRaid( RaidSession* session );
void setRun( RunState* run );
// Loot の欄で触る、漁れる場所（中身を振り直す・全部開けたことにする。R3_Plan.txt Q13）。本編が渡す。所有権は受け取らない。
// nullptr なら欄に何も出さない。シーンが終わるときに nullptr へ戻す。Release では何もしない。
void setInteraction( InteractionSystem* interaction );

} //namespace EditorWindow

#endif
