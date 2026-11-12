#ifndef INCLUDED_BAG_CONTROLLER_H
#define INCLUDED_BAG_CONTROLLER_H

#include "GameSrc/Scene/Game/Screen/InventoryScreen.h"

class Inventory;
class GameWorld;
class FirstPersonControl;
class ModalController;
class UiRenderer;
class IInteractable;
enum class PlayerCondition;

// カバン（持ち物のマス目）の開閉と、開いている間の操作をまとめる
// 開くと世界が止まる（ModalController::Mode::Bag）ので、マスの出し入れ・使う・捨てる・持ち替え・拾った物の置き場所選びはここで完結する
// 開閉の共通遷移（音/カーソル）は ModalController、見た目は InventoryScreen、HUD の弾数や脱出点の文面は GameScene が持つ
class BagController{
public:
	BagController( Inventory* inventory, GameWorld* world, FirstPersonControl* control, ModalController* modals, UiRenderer* ui );	// inventory/world/control/modals: 借りるだけ（所有しない）。ui: 画面の描画に使う。どれも nullptr 不可・この Controller より長く生きていること
	// 開けるか（自キャラが生きている）。観察用カメラ中・死亡の最終判断は呼び出し側で足す
	bool canOpen() const;
	void open();
	void close();
	void startPlacing( IInteractable* offer );																						// E で拾った物 offer を持った状態で開く（入れるマスを選ぶ）
	void handleInput();																												// 開いている間の入力（Tab/ESC で閉じる・マス操作・使う・捨てる・持ち替え）。ほかの入力は読まない
	void draw( const char* exits );																									// カバンの画面を描く。exits: 見出しの下に出す今日の脱出の車の文面（GameScene が RaidSession から作る）

private:
	bool offeredStack( ItemStack* out ) const;	// 拾って持っている物（銃なら弾倉の数も入れる）を out へ。持っていなければ false
	// 拾った物を cell を指した所へ入る分だけ入れる・おまかせで入れる。全部入ったら閉じる
	void placeOffered( int cell );
	void quickPlaceOffered();
	void finishPlacingIfDone();
	void dropFromBag( int cell );				// cell の物を取り出して足元に置く（置けなければ戻す）
	// cell の体力を戻す物（包帯）を 1 つ使う
	void useFromBag( int cell );
	bool isPlayerInjured() const;
	PlayerCondition playerCondition() const;
	ItemId equippedWeapon() const;				// 手に持っている銃の物（素手・自キャラがいなければ ITEM_NONE）。マスの「E」の印と Equip に使う

	Inventory* mInventory;			// 借りているだけ
	GameWorld* mWorld;				// 借りているだけ
	FirstPersonControl* mControl;	// 借りているだけ（捨てる向きのため）
	ModalController* mModals;		// 借りているだけ（開閉の共通遷移）
	InventoryScreen mScreen;
	IInteractable* mOffer;			// E で拾って、入れるマスを選んでいる物（無ければ nullptr）。借りているだけ（開いている間は世界が止まり、拾う物は消えない）
};

#endif
