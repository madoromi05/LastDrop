#ifndef INCLUDED_EDITOR_WINDOW_H
#define INCLUDED_EDITOR_WINDOW_H

// 開発用のエディタの窓（F6 で開く。Debug・Profile ビルドだけ）。
// 画面の右端に上から下まで 1 枚出し、タブで欄を切り替える。画面は暗くせず、ゲームも止めない。
//   Debug ：確認用の切り替え（無敵・ゾンビを止める・ゾンビを出す・当たり判定などの表示・観察用カメラ）。
//           本編へは DebugSwitches（ゾンビを止めるのは zombiesFrozen）を通して頼む
//   Scene ：一時停止・1 ティック進める・時間の倍率（SceneManager の時計。ESC の一時停止とは別で、メニューも音も触らない）
//   Render：VSync の切り替え（CPU と GPU のどちらが重いかを見分けるときに切る）と、昼・夜の光（LightingParams）・影の ON/OFF・G-Buffer と影の中身の表示（DeferredLighting）
//   Perf  ：計測の数字・グラフ・区間・印・ロード・GPU・CSV の記録（中身は GameLib の Profiling::drawPerfPanel。CSV は旧 F7）
//   Tuning：調整値の構造体（TuningPanel。自キャラ・銃・視点・ゾンビの群れ。プリセットの保存・読み込みと Copy as C++）
//   World ：見出しで開け閉めする 4 つの欄
//     Objects：世界に置いた物の一覧（Unity の Hierarchy。名前・タグ・部品の数）
//     Inventory：銃 2 種と弾 2 種を好きな数だけ足す・減らす・カバンを空にする・行を増やす／減らす（拾いに行かずに試すため）
//     Raid：すぐ脱出する・日の条件を一覧から選ぶ（今の出撃にすぐ効き、次の出撃にも引き継ぐ）・すぐ始め直す（出撃の流れをすぐ試すため）
//     Loot：漁れる場所の中身を振り直す・全部開けたことにする
//   Pad   ：パッドの 0 番の今の入力
// ImGui の土台（開け閉め・入力の受け渡し）は GameLib の DebugUi が持つ。ここは窓の中身だけ。
class GameObjectWorld;
class Inventory;
class RaidSession;
class RaidRecord;
class InteractionSystem;
class GameWorld;

namespace EditorWindow{

// エディタが開いているときに、1 回の描画につき 1 回呼ぶ（SceneManager::update の後）。Release では何もしない。
void draw();
// 自キャラが無敵か・観察用カメラを使っているかを、Debug タブのチェックに出すために受け取る。窓は自キャラもカメラも知らないので、GameScene から渡す。
// GameScene が終わるときに false へ戻す（タイトルなどで ON のまま残さない）。Release では何もしない。
void setPlayerInvincible( bool invincible );
void setFreeCameraActive( bool active );
// Objects の欄に一覧を出す物の持ち主。窓は世界を知らないので、GameScene から渡す。所有権は受け取らない。
// nullptr なら欄に何も出さない。GameScene が終わるときに nullptr へ戻す（消えた世界を指し続けないように）。Release では何もしない。
void setObjects( const GameObjectWorld* objects );
// Inventory の欄で触るカバン。窓は自キャラを知らないので、GameScene から渡す。所有権は受け取らない。
// nullptr なら欄に何も出さない。GameScene が終わるときに nullptr へ戻す。Release では何もしない。
void setInventory( Inventory* inventory );
// Raid の欄で触る、1 回の出撃（経った時間・すぐ脱出）と、出撃の間で受け渡す物（次の出撃の条件の固定）。
// どちらも窓は知らないので、本編が渡す。所有権は受け取らない。
// nullptr ならその行を出さない。シーンが終わるときに nullptr へ戻す。Release では何もしない。
void setRaid( RaidSession* session );
void setRaidRecord( RaidRecord* record );
// Loot の欄で触る、漁れる場所（中身を振り直す・全部開けたことにする）。本編が渡す。所有権は受け取らない。
// nullptr なら欄に何も出さない。シーンが終わるときに nullptr へ戻す。Release では何もしない。
void setInteraction( InteractionSystem* interaction );
// Raid の欄で今日の条件を切り替える、今の出撃の世界（GameWorld::changeDayCondition）。本編が渡す。所有権は受け取らない。
// nullptr なら今の出撃には効かせない（次の出撃の固定だけ）。シーンが終わるときに nullptr へ戻す。Release では何もしない。
void setWorld( GameWorld* world );

} //namespace EditorWindow

#endif
