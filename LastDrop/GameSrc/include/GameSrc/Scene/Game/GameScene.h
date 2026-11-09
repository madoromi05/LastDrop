#ifndef INCLUDED_GAME_SCENE_H
#define INCLUDED_GAME_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Scene/Game/World/SceneRenderer.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Player/Camera/FirstPersonControl.h"
#include "GameSrc/Scene/Game/PlayerFeedback.h"
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
#include "GameSrc/Scene/Game/GameEditorBridge.h"
#endif

class GameResources;
class EscapeCarComponent;

// ゲーム本編（1 回勝負の出撃。SummaryPlan/個別仕様/出撃と帰還/SimpleRun_Plan.txt）。自キャラを一人称(FPS)視点で動かし、町でガソリンを探して脱出の車で出る。
// WASD で移動（Shift で走る）、マウスで視点を回す。E で、見ている物を拾う。左クリックを押している間、銃を撃つ。R で弾を込める。
// Escape from Tarkov のように自分の体（腕・銃・脚）が見える方式。視点は頭の骨に付き、歩きの揺れも出る。
// 始めのカバンと今回の条件は RaidStart が決める（毎回同じカバン・毎回ランダムな条件）。給油した車のそばに決まった秒だけ立つと脱出し、結果の画面（クリア）へ。
// ゾンビに攻撃されて体力が 0 になると、視点が下がって暗くなり、結果の画面（死んだ）へ。
// ESC か、窓が裏に回ったら一時停止する（世界と音を止め、PauseMenu を重ねる。もう一度 ESC か Resume で戻る）。死んだ後は止めない
// （崩れて暗くなるのを見せ切ってから結果の画面へ進むため）。
// Tab でカバンを開く（一時停止と同じく世界と音を止め、InventoryScreen を重ねる。もう一度 Tab か ESC で戻る）。
// 漁れる場所（ゴミ箱・木箱）を見て E を押すと開け始め（LootSearch。動く・撃つ・構える・攻撃される・E で取りやめ）、
// 開け終わったら中を見る画面（LootScreen。カバンと同じく世界と音を止める。ESC・E・Tab で戻る）。
//
// このクラスは、部品を「1回の描画の中でどの順に動かすか」だけを決める。中身はそれぞれの部品が持つ:
//   世界（町・キャラ・物・当たり判定）  … GameWorld
//   世界を描く                          … WorldRenderer
//   入力と視点                          … FirstPersonControl
//   見ている物を使う                    … Interactor（持ち物は Inventory）
//   出来事を自キャラへ返す（当たった音・揺れ・攻撃された反応・足音・反動とぶれ）… PlayerFeedback
//     （攻撃された・死んだ後の見せ方の量は PlayerDamageView。画面の赤み・暗さは PostProcess、視点の下がりは FirstPersonControl へ渡す）
//   出撃の始めの形（カバン・条件）      … RaidStart（F6 で固定した条件は RaidRecord）
//   経った時間・脱出の車・終わり方      … RaidSession（結果は RaidRecord に置いて、結果の画面が読む）
//   HUD                                 … RaidHud
//   一時停止のメニュー                  … PauseMenu
//   カバンの画面                        … InventoryScreen
//   漁れる場所を開けている途中・中を見る画面 … LootSearch・LootScreen
//   画面全体の処理                      … PostProcess
//   確認用の表示                        … GameDebugOverlay
//   確認用の観察カメラ（Debug・Profile だけ。F6 の Debug タブ）… FreeCamera
//   F6 のエディタ・F6 から頼まれた無敵とゾンビを出す・FireProbe（Debug・Profile だけ）… GameEditorBridge
// 環境音（脱出のサイレン）は鳴らさない。
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
	// 出撃を 1 ティック進め（RaidSession）、終わったら結果の画面へ移る（1ティックに1回、PlayerFeedback::reactToCombat の後）。
	// 脱出：すぐ。死んだ：暗くなり終わるのを待ってから。
	void updateRaid();
	// 終わった出撃の結果を RaidRecord に置いて、結果の画面への切り替えを予約する（1 回だけ）。
	void finishRaid();
	// ロードの後の、描画ごとの操作の読み取り（一時停止・カバン・中を見る画面・視点と移動）。beginFrame から。
	void handleFrameInput();
	float aimWeight() const;     // 自キャラの覗き具合（0〜1。自キャラがいなければ 0）
	// 画を描き、音を聞くカメラ。観察用カメラ（FreeCamera）を使っている間はそちら、それ以外は自キャラの視点。
	// 自キャラを動かす向き・見ている物を使う視線・ゾンビの湧く場所には使わない（そちらは常に自キャラの視点）。
	const FirstPersonCamera& viewCamera() const;
	bool isFreeCameraActive() const; // 観察用カメラを使っているか（Release では常に false）

	GameResources* mResources;   // 借りているだけ
	// 世界。今回の条件（停電・霧など）も世界が持つ。他の部品が中の物（名簿・地面・町の配置）を借りるので、先に宣言する（＝後に破棄される）。
	GameWorld mWorld;
	// 世界を画面へ写す（ディファードのパイプライン。View）。mWorld を読むので、mWorld より後に宣言する（＝先に破棄される）。
	SceneRenderer mSceneRenderer;
	Inventory mInventory;
	Interactor mInteractor;
	// 1 回の出撃（mWorld の町の脱出の車の候補を借りるので、mWorld より後に宣言する）。
	// 選んだ車のそばの始めの向きを mControl が読むので、mControl より先に宣言する。
	RaidSession mSession;
	FirstPersonControl mControl;
	// このティックの出来事を自キャラの視点・音・画面へ返す（足元の材質を mWorld の町から借りるので、その後に宣言する）。
	PlayerFeedback mFeedback;
	// 遊んでいる間の画面表示（始めの知らせ・弾数・使える物のマーク）。mWorld/mInventory を借りるので、その後に宣言する。
	HudController mHud;
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
	GameEditorBridge mEditor; // F6 のエディタ・F6 から頼まれた無敵とゾンビを出す・FireProbe
#endif
	bool mRaidFinished; // 出撃を終えて次のシーンを予約したか（同じ描画の残りのティックで、もう一度終えないように）
};

#endif
