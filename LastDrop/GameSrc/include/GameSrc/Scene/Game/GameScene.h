#ifndef INCLUDED_GAME_SCENE_H
#define INCLUDED_GAME_SCENE_H

#include "GameSrc/Scene/IScene.h"
#include "GameSrc/Scene/Game/GameWorld.h"
#include "GameSrc/Scene/Game/WorldRenderer.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Player/FirstPersonControl.h"
#include "GameSrc/Player/PlayerBodySounds.h"
#include "GameSrc/Scene/Game/ZombieHitSounds.h"
#include "GameSrc/Scene/Game/SurfaceHitSounds.h"
#include "GameSrc/Scene/Game/ZombieAttackSounds.h"
#include "GameSrc/Scene/Game/PlayerDamageView.h"
#include "GameSrc/Scene/Game/RaidHud.h"
#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Scene/Game/InteractMarkers.h"
#include "GameSrc/Scene/Game/PauseMenu.h"
#include "GameSrc/Scene/Game/InventoryScreen.h"
#include "GameSrc/Scene/Game/LootScreen.h"
#include "GameSrc/Interaction/LootSearch.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/World/GameObjectHandle.h"
#include "GameSrc/Render/PostProcess.h"
#include "GameSrc/Render/GBuffer.h"
#include "GameSrc/Render/DeferredLighting.h"
#include "GameSrc/Render/AmbientOcclusion.h"
#include "GameSrc/Render/ScreenSpaceReflection.h"
#include "GameSrc/Render/TemporalJitter.h"
#include "GameSrc/Render/TemporalAntiAliasing.h"
#include "GameSrc/Render/LightList.h"
#include "GameSrc/Render/ShadowMap.h"
#include "GameSrc/Render/CascadedShadowMap.h"
#include "GameSrc/Render/LightingParams.h"
#include "GameSrc/Scene/Game/GameDebugOverlay.h"
#include "GameSrc/Scene/Game/GameLoader.h"
#include "GameSrc/Scene/LoadingScreen.h"
#ifdef ZF_EDITOR
#include "GameSrc/DebugMode/FreeCamera.h"
#include "GameSrc/DebugMode/FireProbe.h"
#endif

class GameResources;
class DrawCulling;
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
//   ゾンビの腕が当たった音              … ZombieAttackSounds
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
	// カバンを開く・閉じる。止め方は一時停止と同じ（世界を進めず、音を止め、カーソルを外す）。
	// 開けるのは、自キャラが生きていて、観察用カメラを使っていない間だけ。
	bool canOpenBag() const;
	void openBag();
	void closeBag();
	// カバンを開いている間の入力（Tab・ESC で閉じる・マスの操作・捨てた物を足元に置く・銃を持ち替える）。ほかの入力は読まない。
	void handleBagInput();
	// cell の物を取り出して、自キャラの足元に置く（置けなければカバンへ戻す）。
	void dropFromBag( int cell );
	// E で拾った物 offer を持った状態でカバンを開く（入れるマスを選ぶ。InventoryPickup_Plan.txt Q3）。
	void startPlacing( IInteractable* offer );
	// 拾って持っている物（銃なら弾倉の数も入れる）を out へ。持っていなければ false。
	bool offeredStack( ItemStack* out ) const;
	// 拾った物を cell を指した所へ入る分だけ入れる・おまかせで入れる。全部入ったらカバンを閉じる。
	void placeOffered( int cell );
	void quickPlaceOffered();
	// 拾った物を全部入れたら、拾うのを終えてカバンを閉じる。
	void finishPlacingIfDone();
	// カバンの cell の体力を戻す物（包帯）を 1 つ使う（R3_Plan.txt Q11。世界は止まっているので、すぐ効く）。
	void useFromBag( int cell );
	// 体力が減っているか（カバンの画面の「使う」を出すか）。自キャラが居なければ false。
	bool isPlayerInjured() const;
	// ── 漁れる場所（R3_Plan.txt 5 章）──
	// E で漁れる場所 opened を選んだ。開けた所ならすぐ中を見る画面、まだなら開け始める（開けている音を鳴らす）。
	void beginLoot( IInteractable* opened );
	// ── 脱出の車（FuelEscapeLoading_Plan.txt 3 章）──
	// E で脱出の車 used を選んだ。漁るのと同じ待ち方（LootSearch・取りやめも同じ）で、給油し始める。
	void beginRefuel( IInteractable* used );
	// 給油の待ちを終えた。ガソリンを 1 個入れ、要る数に届いたらエンジンを掛ける（音・ゾンビを呼ぶ・脱出の円を開く）。
	void finishRefuel( EscapeCarComponent* car );
	// 開けている途中を取りやめる（音を止める）。開けていなければ何もしない。
	void cancelLootSearch();
	// 今の待ち（mLootSearch）が漁るのか給油なのか（HUD の棒の文字。待っている相手が脱出の車なら給油）。
	RaidHud::ProgressKind waitingKind() const;
	// 開けている途中を 1 ティック進める（update から。commandInterrupts: このティックの操作が取りやめの操作だったか）。
	// 攻撃された・死んだ・1 日が終わったも取りやめ。開け終わったら中を見る画面を開く（脱出の車なら、ガソリンを入れる）。
	void updateLootSearch( bool commandInterrupts );
	// 操作 command が、開けている途中の取りやめになるか（動く・撃つ・構える・リロード・武器を出し入れ）。
	static bool interruptsLootSearch( const PlayerCommand& command );
	// 中を見る画面を開く・閉じる。止め方はカバンと同じ（世界を進めず、音を止め、カーソルを外す）。
	void openLoot( LootContainerComponent* container );
	void closeLoot();
	// 中を見る画面を開いている間の入力（ESC・E・Tab で閉じる・マスの操作）。ほかの入力は読まない。
	void handleLootInput();
	// 中を見ている場所（消えた・開いていなければ nullptr）。
	LootContainerComponent* lootTarget() const;
	// 自キャラが手に持っている銃の物（素手・自キャラがいなければ ITEM_NONE）。カバンの画面の「E」の印と Equip に使う。
	ItemId equippedWeapon() const;
	// 描画の設定（奥行き・カリング）と、今のフレームのライトの一覧（月・補助光・懐中電灯・マズルフラッシュ・街灯・空・環境光）を作って付ける。
	// camera：描くカメラ（目の位置はライトを選ぶ順番、目・向き・画角は月の影の枚の範囲に使う）。
	void setupLights( const FirstPersonCamera& camera );
	// 月（平行光）を一覧へ足す。強さがあれば、影の 3 枚（CSM）の行列を決めて影のマップを付ける（Shadow_Plan.txt 6 章）。
	void addMoon( const LightingParams& params, const FirstPersonCamera& camera );
	// 自キャラの懐中電灯が付いていて銃を持っていれば、銃口の前から銃の向きへ照らすスポットを一覧へ足す（PbrNightLighting_Plan.txt 9 章）。
	void addFlashlight( const LightingParams& params );
	// 影のマップを描く（懐中電灯・月の 3 枚）。setupLights の後、G-Buffer の前に呼ぶ。終わったら描く行列を camera へ戻す。
	void drawShadowMaps( const FirstPersonCamera& camera );
	// 目から見る絵の、描くかどうか（OptimizationA_Plan.txt 6 章）。描く距離は霧から決める（霧を切っている間は距離で間引かない）。
	DrawCulling cameraCulling( const FirstPersonCamera& camera ) const;
	void updateInteraction();    // 見ている物の案内を出し、E で使う。車が走れるようになったらクリア画面へ（1回の描画につき1回）
	void updateListener();       // 聞き手をカメラに合わせて音を更新する（1回の描画につき1回。視点を置いた後）
	// ゾンビの腕が当たった出来事を読み、音を鳴らし、怯ませ、攻撃された・死んだ後の見せ方を進める。
	// 暗くなり終わったらゲームオーバー画面へ（1ティックに1回、GameWorld::update の後）。
	void updatePlayerDamage();
	// 1 日を 1 ティック進め（RaidSession）、終わったら RunRules に頼んで次のシーンへ移る（1ティックに1回、updatePlayerDamage の後）。
	// 脱出：銃の弾倉の数をカバンへ書き戻してから finishDay → 1 日の結果。時間切れ：failDay → 5 日間の終わり。
	// 死んだ：暗くなり終わるのを待ってから failDay → 5 日間の終わり。
	void updateRaid();
	// 終わった日を RunRules へ渡して、次のシーンへの切り替えを予約する（1 回だけ）。
	void finishRaid();
	// カバンの銃のマスへ、銃の物の弾倉の数を書き戻す（脱出したとき。R2_Plan.txt Q13）。
	void storeMagazineRounds();
	// 拠点で覚えた弾倉の数を、カバンの銃の物へ入れる（世界を並べた後に 1 回）。
	void restoreMagazineRounds();
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
	// 世界を描く（View）。mWorld を読むので、mWorld より後に宣言する（＝先に破棄される）。
	WorldRenderer mRenderer;
	Inventory mInventory;
	Interactor mInteractor;
	InteractMarkers mInteractMarkers; // 使える物の上のマーク（文字の案内の代わり）
	FirstPersonControl mControl;
	PlayerBodySounds mBodySounds;
	ZombieHitSounds mZombieHitSounds;
	SurfaceHitSounds mSurfaceHitSounds;
	ZombieAttackSounds mZombieAttackSounds;
	PlayerDamageView mDamageView;
	RaidHud mHud;
	RaidSession mSession; // 町の 1 日（mWorld の町の脱出の車の候補を借りるので、mWorld より後に宣言する）
	// 出撃のロードの段階（mWorld と、mSession が選んだ脱出の車の置き場所を使うので、その後に宣言する）。
	GameLoader mLoader;
	LoadingScreen mLoadingScreen;
	PauseMenu mPauseMenu;
	InventoryScreen mInventoryScreen;
	LootScreen mLootScreen;
	LootSearch mLootSearch;
	PostProcess mPostProcess; // シーンを中間の描画先へ描き、シェーダを通して画面へ写す
	GBuffer mGBuffer;                   // 不透明な物の面の情報（ディファードで描く間。GBuffer_Plan.txt 5 章）
	DeferredLighting mDeferredLighting; // G-Buffer を読んで照らす・中身を見せる
	AmbientOcclusion mAmbientOcclusion; // SSAO（G-Buffer の後に作る。SsaoTranslucent_Plan.txt 5 章）
	ScreenSpaceReflection mReflection;  // 映り込み（ライティングの後に作る。SsrDecalMaterial_Plan.txt 7 章）
	TemporalJitter mJitter;             // TAA の画面のずらし（描くたびに進める。Taa_Plan.txt 5 章）
	TemporalAntiAliasing mTaa;          // TAA（霧の後に混ぜる。Taa_Plan.txt 7 章）
	const FirstPersonCamera* mTaaCamera = nullptr; // 前のフレームに見ていたカメラ（替わったら TAA の履歴を捨てる）。目印だけで読まない
	LightList mLights;                  // 今のフレームのライト（setupLights で作り直す）
	ShadowMap mFlashlightShadow;        // 懐中電灯の影のマップ（Shadow_Plan.txt 5 章）
	bool mFlashlightCastsShadow;        // このフレームに懐中電灯の影を描くか（付いていて銃を持っている間。addFlashlight が決める）
	GameLib::Math::Matrix44 mFlashlightShadowMatrix; // 懐中電灯の影のマップの、光の側の射影×ビュー
	CascadedShadowMap mMoonShadow;      // 月の影の 3 枚（Shadow_Plan.txt 6 章）
	bool mMoonCastsShadow;              // このフレームに月の影を描くか（月に強さがある間。addMoon が決める）
	GameDebugOverlay mDebug;
#ifdef ZF_EDITOR
	FreeCamera mFreeCamera;
	FireProbe mFireProbe; // 1 発目が出ない件の仮の記録（原因が決まったら消す）
#endif
	bool mReady; // 世界の配置が済んだか
	bool mPaused; // 一時停止しているか
	bool mBagOpen; // カバンを開いているか（開いている間も世界は止まっている）
	bool mRaidFinished; // 1 日を終えて次のシーンを予約したか（同じ描画の残りのティックで、もう一度終えないように）
	// E で拾って、入れるマスを選んでいる物（無ければ nullptr）。借りているだけ（カバンを開いている間は世界が止まり、拾う物は消えない）。
	IInteractable* mOffer;
	bool mLootOpen;               // 中を見る画面を開いているか（開いている間も世界は止まっている）
	GameObjectHandle mLootTarget; // 中を見ている場所の物（車・ゴミ箱・木箱・死体）
	AudioEngine::LoopHandle mSearchSound; // 開けている間の音（開けていなければ kInvalidLoop）
	AudioEngine::LoopHandle mEngineSound; // 脱出の車のエンジンの音（掛かっていなければ kInvalidLoop）
};

#endif
