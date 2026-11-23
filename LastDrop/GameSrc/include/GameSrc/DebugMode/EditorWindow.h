#ifndef INCLUDED_EDITOR_WINDOW_H
#define INCLUDED_EDITOR_WINDOW_H

// 開発用のエディタの窓（F6 で開く。Debug・Profile ビルドだけ）
// 画面の右端に上から下まで 1 枚出し、タブで欄を切り替える。画面は暗くせず、ゲームも止めない
//   Debug ：確認用の切り替え（無敵・ゾンビを止める・ゾンビを出す・当たり判定などの表示・自キャラを飛ばす）
//           本編へは DebugSwitches（ゾンビを止めるのは zombiesFrozen）を通して頼む
//   Scene ：一時停止・1 ティック進める・時間の倍率（SceneManager の時計。ESC の一時停止とは別で、メニューも音も触らない）
//           結果画面の確認（時間・倒した数・条件を入れて、脱出成功・ゲームオーバーの画面をすぐ開く。どのシーンからでも）
//   Render：VSync の切り替え（CPU と GPU のどちらが重いかを見分けるときに切る）と、昼・夜の光（LightingParams）・影の ON/OFF・G-Buffer と影の中身の表示（DeferredLighting）
//   Perf  ：計測の数字・グラフ・区間・印・ロード・GPU・CSV の記録（中身は GameLib の Profiling::drawPerfPanel）
//   Tuning：調整値の構造体（TuningPanel。銃・ゾンビ・絵など。プリセットの保存・読み込みと Copy as C++）
//   World ：見出しで開け閉めする 3 つの欄
//     Inventory：銃 2 種と弾 2 種を好きな数だけ足す・減らす・カバンを空にする・行を増やす／減らす（拾いに行かずに試すため）
//     Raid：すぐ脱出する・日の条件を一覧から選ぶ（今の出撃にすぐ効き、次の出撃にも引き継ぐ）・すぐ始め直す（出撃の流れをすぐ試すため）
//     Loot：漁れる場所の中身を振り直す・全部開けたことにする
//   Town  ：町の物を world 座標で手直しする（TownEditPanel。一覧・選んだ物の欄・Save・Revert・Apply）。出撃中だけ使える
//           事前に作る床と表のファイルの状態と書き出し（TownBakePanel。Bake・Bake all）
//   Pad   ：パッドの 0 番の今の入力
// ImGui の土台（開け閉め・入力の受け渡し）は GameLib の DebugUi が持つ。ここは窓の中身だけ
class Inventory;
class RaidSession;
class RaidRecord;
class InteractionSystem;
class GameWorld;
class TownCache;
class TownEdits;
class TownEditFileState;
class FirstPersonCamera;

namespace EditorWindow{

void draw();	// エディタが開いているときに、1 回の描画につき 1 回呼ぶ（SceneManager::update の後）。Release では何もしない
// 自キャラが無敵か・飛んでいるかを、Debug タブのチェックに出すために受け取る。窓は自キャラもカメラも知らないので、GameScene から渡す
// GameScene が終わるときに false・nullptr へ戻す（タイトルなどで ON のまま残さない）。Release では何もしない
void setPlayerInvincible( bool invincible );
void setFlightCamera( const FirstPersonCamera* camera );	// camera: 自キャラが飛んでいる間はその視点（Town タブの Pick と Add が正面の線を使う）、飛んでいなければ nullptr。所有権は受け取らない
// Inventory の欄で触るカバン。窓は自キャラを知らないので、GameScene から渡す。所有権は受け取らない
// nullptr なら欄に何も出さない。GameScene が終わるときに nullptr へ戻す。Release では何もしない
void setInventory( Inventory* inventory );
// Raid の欄で触る、1 回の出撃（経った時間・すぐ脱出）。窓は知らないので、本編が渡す。所有権は受け取らない
// nullptr ならその行を出さない。シーンが終わるときに nullptr へ戻す。Release では何もしない
void setRaid( RaidSession* session );
// Raid の欄（次の出撃の条件の固定）と、Scene タブの結果画面の確認が触る、出撃の間で受け渡す物。所有権は受け取らない
// 起動から終了まで 1 つなので、SceneManager::start が 1 回だけ渡す（どのシーンでも触れる）。Release では何もしない
void setRaidRecord( RaidRecord* record );
// Loot の欄で触る、漁れる場所（中身を振り直す・全部開けたことにする）。本編が渡す。所有権は受け取らない
// nullptr なら欄に何も出さない。シーンが終わるときに nullptr へ戻す。Release では何もしない
void setInteraction( InteractionSystem* interaction );
// Raid の欄で今日の条件を切り替える、今の出撃の世界（GameWorld::changeDayCondition）。本編が渡す。所有権は受け取らない
// nullptr なら今の出撃には効かせない（次の出撃の固定だけ）。シーンが終わるときに nullptr へ戻す。Release では何もしない
void setWorld( GameWorld* world );
// Town タブの Bake が、歩ける床と隠れの表を作って書き出す所。所有権は受け取らない
// 起動から終了まで 1 つなので、SceneManager::start が 1 回だけ渡す。nullptr なら押せない。Release では何もしない
void setTownCache( TownCache* cache );
// Town タブが書き換える町の手直しと、その TownEdit.xml との間の様子（どちらも GameResources が持つ 1 つ）。所有権は受け取らない
// 起動から終了まで 1 つなので、SceneManager::start が 1 回だけ渡す。nullptr なら欄に何も出さない。Release では何もしない
void setTownEdits( TownEdits* edits, TownEditFileState* file );

} //namespace EditorWindow

#endif
