#ifndef INCLUDED_INVENTORY_COMPONENT_H
#define INCLUDED_INVENTORY_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/Item/Inventory.h"

class GameObject;

// 自キャラの持ち物（カバン。中身は Inventory。SummaryPlan/個別仕様/設計と分割/InventoryComponent_Plan.txt）
// 持ち物を持つだけで、自分では動かない。中の出し入れは、持ち物を引数で受け取る側が行う
// （リロードは GunComponent、拾うのは PickupComponent、カバンの画面は BagController）
// 自キャラに必須の部品（PlayerComponent が start で借りる）
class InventoryComponent : public Component{
public:
	explicit InventoryComponent( const ItemGrid& bag );	// bag: 始めのカバンの中身（RaidStart::startBag。写して持つ）

	Inventory& inventory();
	const Inventory& inventory() const;
	// holder（自キャラ）の持ち物。holder が nullptr・この部品が付いていなければ nullptr
	// 物の外から持ち物を読む所（シーンの HUD・カバンの画面・漁る画面・F6 のエディタ）が、同じ 3 行を書かずに済むようにする
	static Inventory* inventoryOf( GameObject* holder );
	static const Inventory* inventoryOf( const GameObject* holder );

private:
	Inventory mInventory;
};

#endif
