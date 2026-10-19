#ifndef INCLUDED_GAME_SCENE_H
#define INCLUDED_GAME_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Player/FirstPersonControl.h"
#include "GameSrc/Player/PlayerBodySounds.h"
#include "GameSrc/Scene/Game/ZombieHitSounds.h"
#include "GameSrc/Scene/Game/GameHud.h"
#include "GameSrc/Render/PostProcess.h"
#include "GameSrc/Scene/Game/GameDebugOverlay.h"

class GameResources;

// ゲーム本編。自キャラを一人称(FPS)視点で動かし、ガソリンを集めて車で脱出する。
// WASD で移動（Shift で走る）、マウスで視点を回す。E で、見ている物を拾う・車に給油する。左クリックを押している間、銃を撃つ。
// Escape from Tarkov のように自分の体（腕・銃・脚）が見える方式。視点は頭の骨に付き、歩きの揺れも出る。
// 車にガソリンを決まった数だけ入れるとクリア画面へ進む。
//
// このクラスは、部品を「1回の描画の中でどの順に動かすか」だけを決める。中身はそれぞれの部品が持つ:
//   世界（町・キャラ・物・当たり判定）  … GameWorld
//   入力と視点                          … FirstPersonControl
//   見ている物を使う                    … Interactor（持ち物は Inventory）
//   足音・装備の音                      … PlayerBodySounds
//   弾がゾンビに当たった音              … ZombieHitSounds
//   HUD                                 … GameHud
//   画面全体の処理                      … PostProcess
//   確認用の表示                        … GameDebugOverlay
// 環境音（AmbienceSystem）は今は鳴らしていない。クラスは残してあるので、メンバに戻せば再び鳴る。
class GameScene : public IScene{
public:
	// resources: メッシュ・音・UI の道具を借りる。所有権は受け取らない。nullptr 不可。このシーンより長く生きていること。
	explicit GameScene( GameResources* resources );
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
private:
	void setupLights() const;
	void updateInteraction();    // 見ている物の案内を出し、E で使う。車が走れるようになったらクリア画面へ（1回の描画につき1回）
	void updateListener();       // 聞き手をカメラに合わせて音を更新する（1回の描画につき1回。視点を置いた後）
#ifdef _DEBUG
	// F9（InputDebugSpawnZombie）で、自キャラの前にゾンビを横並びで出す（確認用。Release にはキーも処理も残さない）。
	// 世界を変えるので、読むだけの GameDebugOverlay ではなくここで行う。beginFrame から（ティックの間に呼ぶ）。
	void spawnDebugZombies();
#endif
	float aimWeight() const;     // 自キャラの覗き具合（0〜1。自キャラがいなければ 0）

	GameResources* mResources;   // 借りているだけ
	// 世界。他の部品が中の物（名簿・地面・町の配置）を借りるので、先に宣言する（＝後に破棄される）。
	GameWorld mWorld;
	Inventory mInventory;
	Interactor mInteractor;
	FirstPersonControl mControl;
	PlayerBodySounds mBodySounds;
	ZombieHitSounds mZombieHitSounds;
	GameHud mHud;
	PostProcess mPostProcess; // シーンを中間の描画先へ描き、シェーダを通して画面へ写す
	GameDebugOverlay mDebug;
	bool mReady; // 世界の配置が済んだか
};

#endif
