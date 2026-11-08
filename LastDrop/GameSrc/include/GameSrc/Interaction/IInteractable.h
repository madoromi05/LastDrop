#ifndef INCLUDED_I_INTERACTABLE_H
#define INCLUDED_I_INTERACTABLE_H

#include "GameSrc/Interaction/Item/ItemGrid.h"

class Inventory;

// 見て操作キー（E）で使える物（落ちている物を拾う など）。
// 使う処理は、Interactor がこの同じ入口から呼ぶ（新しい「使える物」を足しても Interactor は変わらない）。
// 拾う物は、すぐ使わずに「差し出す」（offeredStack）。どのマスへ入れるかを遊ぶ人がカバンの画面で選び、入れた分だけ takeOffered で受け取る
// （InventoryPickup_Plan.txt Q3。2026-11-05 に、自動で入れる形から変えた）。
// 漁れる場所（ゴミ箱・木箱・死体）は、中のマス目（lootGrid）を見せる。開けるまでの時間と中を見る画面は呼ぶ側
// （LootSearch・LootScreen。SummaryPlan/個別仕様/R3_Plan.txt 5 章）。
// 画面には文字の案内を出さず、物の上にマークだけを出す（InteractMarkers。SummaryPlan/個別仕様/InteractMarker_Plan.txt）。
class IInteractable{
public:
	virtual ~IInteractable(){}
	// 今の持ち物で使えるか（使えなければマークを灰色にする。カバンが一杯・ガソリンを持っていない・満タン など）。
	virtual bool canInteract( const Inventory& inventory ) const = 0;
	// 使う。canInteract が true のときだけ呼ばれる。拾う物では、置く所を選ばずにおまかせで入れる（QuickPlace）。
	virtual void interact( Inventory* inventory ) = 0;
	// 拾う物なら、拾える物と数を out に書いて true（置く所を選ぶ画面へ回す）。拾う物でなければ false（Interactor がすぐ interact する）。
	virtual bool offeredStack( ItemStack* out ) const = 0;
	// offeredStack の物を count 個、持ち物へ入れた（入れるのは呼ぶ側）。count は 1〜差し出した数。全部入ったら、もう差し出さない。
	virtual void takeOffered( int count ) = 0;
	// 漁れる場所・脱出の車の給油口なら、中身のマス目（Interactor は開ける物として呼ぶ側へ回す）。どちらでもなければ nullptr。
	virtual ItemGrid* lootGrid() = 0;
};

#endif
