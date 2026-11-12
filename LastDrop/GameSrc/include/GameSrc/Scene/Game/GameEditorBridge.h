#ifndef INCLUDED_GAME_EDITOR_BRIDGE_H
#define INCLUDED_GAME_EDITOR_BRIDGE_H

#include "GameSrc/DebugMode/FireProbe.h"

class GameWorld;
class Inventory;
class RaidSession;
class RaidRecord;
class FirstPersonCamera;
class FirstPersonControl;
class ModalController;

// 本編のシーンと、確認用の道具（F6 のエディタ・F6 から頼まれた無敵とゾンビを出す・FireProbe）をつなぐ係
// （SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 7-2）
// GameScene は ZF_EDITOR のとき（Debug・Profile）だけこれを持つ（Release には持たない。ファイルは他の確認用の道具と同じく、どの構成でもビルドする）
// 観察用カメラ（FreeCamera）と確認用の表示（GameDebugOverlay）は持たない（どのカメラで描くか・パイプラインの途中に挟む、が描画の順番そのものなので、シーンに残す）
class GameEditorBridge{
public:
	GameEditorBridge();
	// シーンを抜けるので、エディタへ渡した物を外す（消えた世界・カバンを指し続けないように）。無敵の表示も OFF に戻す
	~GameEditorBridge();
	GameEditorBridge( const GameEditorBridge& ) = delete;
	GameEditorBridge& operator=( const GameEditorBridge& ) = delete;

	void markReady();							// 出撃のロードが終わった（FireProbe が、そこから記録を始める）
	// 描画ごとに、F6 で頼まれた自キャラの無敵の切り替え（DebugSwitches::toggleInvincible）を行い、F6 のエディタへ今の物を渡す（エディタの欄が読む・書く）
	// 世界・カバン・出撃・出撃の間で受け渡す物はどれも借りるだけ（このシーンの間だけ指す）。freeCamera: 観察用カメラを使っているか（F6 に出す）
	void publish( GameWorld* world, Inventory* inventory, RaidSession* session, RaidRecord* record, bool freeCamera );
	// F6 で頼まれていたら（DebugSwitches::spawnZombies）、自キャラの前にゾンビを出す（世界を変えるので、読むだけの GameDebugOverlay ではなくここ）
	// ティックの間に呼ぶ。camera: 自キャラの視点（向いている方へ出す）
	void spawnRequestedZombies( GameWorld* world, const FirstPersonCamera& camera );
	// 撃つボタンを押した・離した描画を FireProbe へ渡す（1 発目が出ない件の仮の記録）
	// freeCamera: 観察用カメラを使っているか
	void probeFrame( const GameWorld& world, const FirstPersonControl& control, const ModalController& modals, bool freeCamera );
	void probeTick( const GameWorld& world );	// このティックに撃った・空撃ちしたかを FireProbe へ渡す（GameWorld::update の後）

private:
	FireProbe mFireProbe; // 1 発目が出ない件の仮の記録（原因が決まったら消す）
};

#endif
