#ifndef INCLUDED_GAME_SCENE_H
#define INCLUDED_GAME_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Scene/Game/World/SceneRenderer.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Player/Camera/FirstPersonControl.h"
#include "GameSrc/Player/PlayerBodySounds.h"
#include "GameSrc/Scene/Game/Sound/CombatSounds.h"
#include "GameSrc/Scene/Game/Hud/PlayerDamageView.h"
#include "GameSrc/Scene/Game/Hud/HudController.h"
#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Scene/Game/Screen/ModalController.h"
#include "GameSrc/Scene/Game/Screen/PauseMenu.h"
#include "GameSrc/Scene/Game/Screen/BagController.h"
#include "GameSrc/Scene/Game/Screen/LootController.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/World/GameObjectHandle.h"
#include "GameSrc/Scene/Game/Hud/GameDebugOverlay.h"
#include "GameSrc/Scene/Game/LoadingPhase.h"
enum class PlayerCondition;

#ifdef ZF_EDITOR
#include "GameSrc/DebugMode/FreeCamera.h"
#include "GameSrc/DebugMode/FireProbe.h"
#endif

class GameResources;
class EscapeCarComponent;

// ゲーム本編（町の 1 日＝1 回の出撃。SummaryPlan/個別仕様/R2_Plan.txt）。自キャラを一人称(FPS)視点で動かし、町で漁って脱出点から帰る。
// WASD で移動（Shift で走る）、マウスで視点を回す。E で、見ている物を拾う。左クリックを押している間、銃を撃つ。R で弾を込める。
// Escape from Tarkov のように自分の体（腕・銃・脚）が見える方式。視点は頭の骨に付き、歩きの揺れも出る。
// 持ち物は拠点のカバン（RunState）の写しで始まる。開いている脱出点の円に決まった秒だけ立つと脱出し、1 日の結果の画面へ進む。
// ゾンビに攻撃されて体力が 0 になると、視点が下がって暗くなり、5 日間の終わり（ゲームオーバー）の画面へ。時間切れ（行方不明）も同じ。
// ESC か、窓が裏に回ったら一時停止する（世界と音を止め、PauseMenu を重ねる。もう一度 ESC か Resume で戻る）。死んだ後は止めない
// （途中でやめると、その日をやり直せるので、死んでからやめて逃げられないように。R2_Plan.txt Q3）。
// Tab でカバンを開く（一時停止と同じく世界と音を止め、InventoryScreen を重ねる。もう一度 Tab か ESC で戻る。GridInventory_Plan.txt）。
// 漁れる場所（車・ゴミ箱・木箱・死体）を見て E を押すと開け始め（LootSearch。動く・撃つ・構える・攻撃される・E で取りやめ）、
// 開け終わったら中を見る画面（LootScreen。カバンと同じく世界と音を止める。ESC・E・Tab で戻る）。SummaryPlan/個別仕様/R3_Plan.txt。
//
// このクラスは、部品を「1回の描画の中でどの順に動かすか」だけを決める。中身はそれぞれの部品が持つ:
//   世界（町・キャラ・物・当たり判定）  … GameWorld
//   世界を描く                          … WorldRenderer
//   入力と視点                          … FirstPersonControl
//   見ている物を使う                    … Interactor（持ち物は Inventory）
//   足音・装備の音                      … PlayerBodySounds
//   弾がゾンビに当たった音              … ZombieHitSounds
//   弾が物（建物・地面・車）に当たった音… SurfaceHitSounds
//   ゾンビがつかんだ・噛んだ音          … ZombieAttackSounds
//   攻撃された・死んだ後の見せ方の量    … PlayerDamageView（画面の赤み・暗さは PostProcess、視点の下がりは FirstPersonControl へ渡す）
//   1 日の時間・脱出点・終わり方        … RaidSession（終わった後に何を残すかは RunRules）
//   HUD                                 … RaidHud
//   一時停止のメニュー                  … PauseMenu
//   カバンの画面                        … InventoryScreen
//   漁れる場所を開けている途中・中を見る画面 … LootSearch・LootScreen
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
	// カバン（持ち物のマス目）は BagController、漁れる場所・給油は LootController、遊び中の画面表示は HudController が持つ。
	// カバンを開けるのは自キャラが生きていて観察用カメラを使っていない間だけ（handleFrameInput で判定）。
	// 自キャラが手に持っている銃の物（素手・自キャラがいなければ ITEM_NONE）。各画面のマスの印に使う。
	ItemId equippedWeapon() const;
	void updateInteraction();    // 見ている物の案内を出し、E で使う。車が走れるようになったらクリア画面へ（1回の描画につき1回）
	void updateListener();       // 聞き手をカメラに合わせて音を更新する（1回の描画につき1回。視点を置いた後）
	// ゾンビがつかんだ・噛んだ出来事を読み、顔を向け、音を鳴らし、怯ませ、攻撃された・死んだ後の見せ方を進める。
	// 暗くなり終わったらゲームオーバー画面へ（1ティックに1回、GameWorld::update の後）。
	void updatePlayerDamage();
	// このティックの命中のうち、部位が飛んだ・頭が弾けた物だけ画面を少し揺らす（ふつうと倒しただけは揺らさない。HitFeel_Plan.txt Q3）。
	// 1ティックに1回、GameWorld::update の後。
	void shakeOnHeavyHits();
	// 1 日を 1 ティック進め（RaidSession）、終わったら RunRules に頼んで次のシーンへ移る（1ティックに1回、updatePlayerDamage の後）。
	// 脱出：銃の弾倉の数をカバンへ書き戻してから finishDay → 1 日の結果。時間切れ：failDay → 5 日間の終わり。
	// 死んだ：暗くなり終わるのを待ってから failDay → 5 日間の終わり。
	void updateRaid();
	// 終わった日を RunRules へ渡して、次のシーンへの切り替えを予約する（1 回だけ）。
	void finishRaid();
#ifdef ZF_EDITOR
	// F9（InputDebugSpawnZombie）で、自キャラの前にゾンビを横並びで出す（確認用。Release にはキーも処理も残さない）。
	// 世界を変えるので、読むだけの GameDebugOverlay ではなくここで行う。beginFrame から（ティックの間に呼ぶ）。
	void spawnDebugZombies();
	// 撃つボタンを押した・離した描画を FireProbe へ渡す（1 発目が出ない件の仮の記録。FuelEscapeLoading_Plan.txt F-0）。
	void probeFire();
#endif
	// ロードの後の、描画ごとの操作の読み取り（一時停止・カバン・中を見る画面・視点と移動）。beginFrame から。
	void handleFrameInput();
	float aimWeight() const;     // 自キャラの覗き具合（0〜1。自キャラがいなければ 0）
	// 画を描き、音を聞くカメラ。観察用カメラ（FreeCamera）を使っている間はそちら、それ以外は自キャラの視点。
	// 自キャラを動かす向き・見ている物を使う視線・ゾンビの湧く場所には使わない（そちらは常に自キャラの視点）。
	const FirstPersonCamera& viewCamera() const;
	bool isFreeCameraActive() const; // 観察用カメラを使っているか（Release では常に false）

	GameResources* mResources;   // 借りているだけ
	// 世界。他の部品が中の物（名簿・地面・町の配置）を借りるので、先に宣言する（＝後に破棄される）。
	GameWorld mWorld;
	// 世界を画面へ写す（ディファードのパイプライン。View）。mWorld を読むので、mWorld より後に宣言する（＝先に破棄される）。
	SceneRenderer mSceneRenderer;
	Inventory mInventory;
	Interactor mInteractor;
	FirstPersonControl mControl;
	PlayerBodySounds mBodySounds;
	CombatSounds mCombatSounds; // 戦闘の結果の効果音（ゾンビ命中・物命中・攻撃された）をまとめて鳴らす
	PlayerDamageView mDamageView;
	// 遊んでいる間の画面表示（1 日の棒・弾数・使える物のマーク）。mWorld/mInventory を借りるので、その後に宣言する。
	HudController mHud;
	RaidSession mSession; // 町の 1 日（mWorld の町の脱出の車の候補を借りるので、mWorld より後に宣言する）
	// 出撃のロードの段階（mWorld と、mSession が選んだ脱出の車の置き場所を使うので、その後に宣言する）。
	LoadingPhase mLoad;
	PauseMenu mPauseMenu;
	// 全画面のモーダル（一時停止・カバン・漁り）の状態と、開閉の共通遷移（音を止める・カーソル）。mControl と音を借りるので、その後に宣言する。
	ModalController mModals;
	// カバン（持ち物のマス目）の開閉と操作。mInventory/mWorld/mControl/mModals を借りるので、その後に宣言する。
	BagController mBag;
	// 漁れる場所・給油の待ちと中を見る画面。mWorld/mInventory/音/mModals/mSession を借りるので、その後に宣言する。
	LootController mLoot;
	GameDebugOverlay mDebug;
#ifdef ZF_EDITOR
	FreeCamera mFreeCamera;
	FireProbe mFireProbe; // 1 発目が出ない件の仮の記録（原因が決まったら消す）
#endif
	bool mRaidFinished; // 1 日を終えて次のシーンを予約したか（同じ描画の残りのティックで、もう一度終えないように）
};

#endif
