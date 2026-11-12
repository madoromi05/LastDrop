#ifndef INCLUDED_LOOT_CONTROLLER_H
#define INCLUDED_LOOT_CONTROLLER_H

#include "GameSrc/Scene/Game/Screen/LootScreen.h"
#include "GameSrc/Interaction/Loot/LootSearch.h"
#include "GameSrc/Scene/Game/Hud/RaidHud.h"
#include "GameSrc/World/GameObjectHandle.h"
#include "GameSrc/Audio/AudioEngine.h"

class GameWorld;
class Inventory;
class ModalController;
class RaidSession;
class UiRenderer;
class IInteractable;
class GameObject;
class EscapeCarComponent;
class LootContainerComponent;
struct PlayerCommand;

// 漁れる場所（ゴミ箱・木箱）と脱出の車の給油を受け持つ
// E で選んだら開け始め（探る音・動く/撃つ/攻撃されたら取りやめ）、開け終わったら中を見る画面（世界は止まる）
// 給油口は待たずに開き、要る数を入れて閉じたら給油→エンジン始動（脱出の円を開く）。R3_Plan.txt・FuelEscapeLoading_Plan.txt
// 開閉の共通遷移は ModalController、弾倉同期は MagazineSync、見た目は LootScreen。HUD の棒と見出しは GameScene が本クラスへ問い合わせて描く
class LootController{
public:
	LootController( GameWorld* world, Inventory* inventory, AudioEngine* audio, ModalController* modals, RaidSession* session, UiRenderer* ui );	// world/inventory/audio/modals/session: 借りるだけ。ui: 画面の描画に使う。どれも nullptr 不可・この Controller より長く生きていること
	~LootController();																																// 開けている間の音・エンジンの音を止める（音の道具は次のシーンも使う）
	void beginLoot( IInteractable* opened );																										// E で漁れる場所・脱出の車 opened を選んだ。開けた所ならすぐ中を見る画面、まだなら開け始める。脱出の車は待たずに給油口を開く
	// 開けている途中を取りやめる（音を止める）。開けていなければ何もしない
	void cancelSearch();
	bool isSearching() const;
	// update から 1 ティック進める。commandInterrupts: このティックの操作が取りやめか。raidFinished: 1 日が終わったか
	// 攻撃された・死んだも取りやめ。開け終わったら中を見る画面を開く（脱出の車なら、ガソリンを入れる）
	void update( bool commandInterrupts, bool raidFinished );
	static bool interrupts( const PlayerCommand& command );																							// 操作 command が開けている途中の取りやめになるか（動く・撃つ・構える・リロード・武器出し入れ）
	void handleInput();																																// 中を見る画面の入力（ESC・E・Tab で閉じる・マス操作）。ほかの入力は読まない
	void draw( ItemId equippedWeapon );																												// 中を見る画面を描く。equippedWeapon: 手に持っている銃（マスの印に使う。GameScene が持つ）
	bool hasOpenGrid() const;																														// 中を見る画面が開いていて、中身のマス目があるか（draw の前提。消えた物を指していれば false）
	// HUD の進捗（0〜1。isSearching のときだけ意味がある）と、その文字（漁る/給油）
	float progress() const;
	RaidHud::ProgressKind waitingKind() const;

private:
	void beginRefuel( EscapeCarComponent* car );	// 給油口に要る数を入れて閉じた。漁るのと同じ待ち方で給油し始める
	void finishRefuel( EscapeCarComponent* car );	// 給油の待ちを終えた。給油口のガソリンを使ってエンジンを掛ける（音・ゾンビを呼ぶ・脱出の円を開く）
	// 中を見る画面を開く・閉じる（止め方はカバンと同じ）。閉じて給油口が満タンなら給油し始める
	void openLoot( GameObject* target, GridRule lootRule );
	void closeLoot();
	// 中を見ている場所・給油口の車（消えた・開いていなければ nullptr）
	LootContainerComponent* lootTarget() const;
	EscapeCarComponent* fuelPortTarget() const;
	// 中を見ているマス目と、右の見出し（消えた・開いていなければ nullptr）
	ItemGrid* openedGrid() const;
	const char* openedTitle() const;

	GameWorld* mWorld;         // 借りているだけ
	Inventory* mInventory;     // 借りているだけ
	AudioEngine* mAudio;       // 借りているだけ
	ModalController* mModals;  // 借りているだけ（開閉の共通遷移）
	RaidSession* mSession;     // 借りているだけ（給油で脱出の円を開く）
	LootScreen mScreen;
	LootSearch mSearch;
	GameObjectHandle mTarget; // 中を見ている場所の物（ゴミ箱・木箱）
	AudioEngine::LoopHandle mSearchSound; // 開けている間の音（開けていなければ kInvalidLoop）
	AudioEngine::LoopHandle mEngineSound; // 脱出の車のエンジンの音（掛かっていなければ kInvalidLoop）
};

#endif
