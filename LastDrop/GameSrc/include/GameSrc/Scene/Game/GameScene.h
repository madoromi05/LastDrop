#ifndef INCLUDED_GAME_SCENE_H
#define INCLUDED_GAME_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Player/FirstPersonControl.h"
#include "GameSrc/Player/PlayerBodySounds.h"
#include "GameSrc/Scene/Game/ZombieHitSounds.h"
#include "GameSrc/Scene/Game/SurfaceHitSounds.h"
#include "GameSrc/Scene/Game/ZombieAttackSounds.h"
#include "GameSrc/Scene/Game/PlayerDamageView.h"
#include "GameSrc/Scene/Game/GameHud.h"
#include "GameSrc/Scene/Game/PauseMenu.h"
#include "GameSrc/Render/PostProcess.h"
#include "GameSrc/Scene/Game/GameDebugOverlay.h"
#ifdef ZF_EDITOR
#include "GameSrc/DebugMode/FreeCamera.h"
#endif

class GameResources;

// ゲーム本編。自キャラを一人称(FPS)視点で動かし、ガソリンを集めて車で脱出する。
// WASD で移動（Shift で走る）、マウスで視点を回す。E で、見ている物を拾う・車に給油する。左クリックを押している間、銃を撃つ。R で弾を込める。
// Escape from Tarkov のように自分の体（腕・銃・脚）が見える方式。視点は頭の骨に付き、歩きの揺れも出る。
// 車にガソリンを決まった数だけ入れるとクリア画面へ進む。
// ゾンビに噛まれて頭か胸の体力が 0 になると、視点が下がって暗くなり、ゲームオーバー画面へ進む。
// ESC か、窓が裏に回ったら一時停止する（世界と音を止め、PauseMenu を重ねる。もう一度 ESC か Resume で戻る）。
//
// このクラスは、部品を「1回の描画の中でどの順に動かすか」だけを決める。中身はそれぞれの部品が持つ:
//   世界（町・キャラ・物・当たり判定）  … GameWorld
//   入力と視点                          … FirstPersonControl
//   見ている物を使う                    … Interactor（持ち物は Inventory）
//   足音・装備の音                      … PlayerBodySounds
//   弾がゾンビに当たった音              … ZombieHitSounds
//   弾が物（建物・地面・車）に当たった音… SurfaceHitSounds
//   ゾンビがつかんだ・噛んだ音          … ZombieAttackSounds
//   噛まれた・死んだ後の見せ方の量      … PlayerDamageView（画面の赤み・暗さは PostProcess、視点の下がり・顔を向けるのは FirstPersonControl へ渡す）
//   HUD                                 … GameHud
//   一時停止のメニュー                  … PauseMenu
//   画面全体の処理                      … PostProcess
//   確認用の表示                        … GameDebugOverlay
//   確認用の観察カメラ（Debug・Profile だけ。F4）… FreeCamera
// 環境音（AmbienceSystem）は今は鳴らしていない。クラスは残してあるので、メンバに戻せば再び鳴る。
class GameScene : public IScene{
public:
	// resources: メッシュ・音・UI の道具を借りる。所有権は受け取らない。nullptr 不可。このシーンより長く生きていること。
	explicit GameScene( GameResources* resources );
	// 一時停止のまま別のシーンへ移る（Retry・Title）ときに、止めた音を戻す（音の道具は次のシーンも使う）。
	virtual ~GameScene();
	virtual void beginFrame();
	virtual void update();
	virtual void draw();
	// メッシュの組み立てが終わって世界を並べるまでは読み込み中（draw は何も描かない）。
	virtual bool isLoading() const;
private:
	// 一時停止する・戻る。止める間は世界を進めず、音を止め、カーソルを外す。戻ったらカーソルをロックする。
	void pause();
	void resume();
	// 止めている間の入力（ESC で戻る・メニューの操作）。止めている間は、ほかの入力を読まない。
	void handlePausedInput();
	void setupLights() const;
	void updateInteraction();    // 見ている物の案内を出し、E で使う。車が走れるようになったらクリア画面へ（1回の描画につき1回）
	void updateListener();       // 聞き手をカメラに合わせて音を更新する（1回の描画につき1回。視点を置いた後）
	// ゾンビがつかんだ・噛んだ出来事を読み、音を鳴らし、つかまれたら視点をその顔へ向け、噛まれた・死んだ後の見せ方を進める。
	// 暗くなり終わったらゲームオーバー画面へ（1ティックに1回、GameWorld::update の後）。
	void updatePlayerDamage();
#ifdef ZF_EDITOR
	// F9（InputDebugSpawnZombie）で、自キャラの前にゾンビを横並びで出す（確認用。Release にはキーも処理も残さない）。
	// 世界を変えるので、読むだけの GameDebugOverlay ではなくここで行う。beginFrame から（ティックの間に呼ぶ）。
	void spawnDebugZombies();
#endif
	float aimWeight() const;     // 自キャラの覗き具合（0〜1。自キャラがいなければ 0）
	// 画を描き、音を聞くカメラ。観察用カメラ（FreeCamera）を使っている間はそちら、それ以外は自キャラの視点。
	// 自キャラを動かす向き・見ている物を使う視線・ゾンビの湧く場所には使わない（そちらは常に自キャラの視点）。
	const FirstPersonCamera& viewCamera() const;
	bool isFreeCameraActive() const; // 観察用カメラを使っているか（Release では常に false）

	GameResources* mResources;   // 借りているだけ
	// 世界。他の部品が中の物（名簿・地面・町の配置）を借りるので、先に宣言する（＝後に破棄される）。
	GameWorld mWorld;
	Inventory mInventory;
	Interactor mInteractor;
	FirstPersonControl mControl;
	PlayerBodySounds mBodySounds;
	ZombieHitSounds mZombieHitSounds;
	SurfaceHitSounds mSurfaceHitSounds;
	ZombieAttackSounds mZombieAttackSounds;
	PlayerDamageView mDamageView;
	GameHud mHud;
	PauseMenu mPauseMenu;
	PostProcess mPostProcess; // シーンを中間の描画先へ描き、シェーダを通して画面へ写す
	GameDebugOverlay mDebug;
#ifdef ZF_EDITOR
	FreeCamera mFreeCamera;
#endif
	bool mReady; // 世界の配置が済んだか
	bool mPaused; // 一時停止しているか
};

#endif
