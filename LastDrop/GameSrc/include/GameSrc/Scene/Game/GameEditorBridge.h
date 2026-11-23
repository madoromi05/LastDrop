#ifndef INCLUDED_GAME_EDITOR_BRIDGE_H
#define INCLUDED_GAME_EDITOR_BRIDGE_H

#include "GameSrc/DebugMode/TownEditModelMover.h"

class GameWorld;
class RaidSession;
class FirstPersonCamera;
class TownEdits;

// 本編のシーンと、確認用の道具（F6 のエディタ・F6 から頼まれた無敵・飛ぶ・ゾンビを出す）をつなぐ係
// GameScene は ZF_EDITOR のとき（Debug・Profile）だけこれを持つ（Release には持たない。ファイルは他の確認用の道具と同じく、どの構成でもビルドする）
// 確認用の表示（GameDebugOverlay）は持たない（パイプラインの途中に挟む、が描画の順番そのものなので、シーンに残す）
class GameEditorBridge{
public:
	GameEditorBridge();
	// シーンを抜けるので、エディタへ渡した物を外す（消えた世界・カバンを指し続けないように）。無敵の表示も OFF に戻す
	~GameEditorBridge();
	GameEditorBridge( const GameEditorBridge& ) = delete;
	GameEditorBridge& operator=( const GameEditorBridge& ) = delete;

	// 描画ごとに、F6 で頼まれた自キャラの無敵と飛ぶの切り替え（DebugSwitches::toggleInvincible・toggleFlight）を行い、F6 のエディタへ今の物を渡す（エディタの欄が読む・書く）
	// 世界・出撃はどちらも借りるだけ（このシーンの間だけ指す）。カバンは世界の自キャラの持ち物を渡す
	// view: 自キャラの視点。飛んでいる間だけ F6 へ渡す（Town タブの Pick と Add が正面の線を使う）。借りるだけ
	// 出撃の間で受け渡す物（RaidRecord）は起動から終了まで生きるので、ここではなく SceneManager::start が渡す
	void publish( GameWorld* world, RaidSession* session, const FirstPersonCamera& view );
	// F6 で頼まれていたら（DebugSwitches::spawnZombies）、自キャラの前にゾンビを出す（世界を変えるので、読むだけの GameDebugOverlay ではなくここ）
	// ティックの間に呼ぶ。camera: 自キャラの視点（向いている方へ出す）
	void spawnRequestedZombies( GameWorld* world, const FirstPersonCamera& camera );
	// F6 の Town タブの手直しを、町の建物と置物の見た目へその場で写す（TownEditModelMover。世界を変えるのでここ）
	// 描画ごとに、世界を読み終えた後で呼ぶ。edits: 起動中ずっとある町の手直し（GameResources）。読むだけ
	void followTownEdits( GameWorld* world, const TownEdits& edits );

private:
	TownEditModelMover mTownModels;	// この世界の町の物と手直しの対応（世界と同じだけ生きる）
};

#endif
