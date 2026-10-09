#ifndef INCLUDED_INVENTORY_H
#define INCLUDED_INVENTORY_H

#include "ItemCatalog.h"

// 持ち物（何を何個持っているか）。
// 外から使う入口は canAdd / add / remove / count の4つだけにしてある。後でインベントリを作り直す（マス目・重さ・画面など）ときは、
// この4つを残せば、拾う物（Pickup）・車（EscapeCar）・HUD は変えなくてよい。中身の持ち方は外に見せない。
// 何個まで持てるかの決まりは ItemCatalog が持つ（ここには書かない）。
class Inventory{
public:
	Inventory();

	// id をもう1つ持てるか。
	bool canAdd( ItemId id ) const;
	// id を1つ持つ。canAdd が false のときは呼ばないこと（持ちきれない分を黙って捨てないよう、ASSERT で止める）。
	void add( ItemId id );
	// id を1つ手放す。持っていないときは呼ばないこと（ASSERT で止める）。
	void remove( ItemId id );
	// id をいくつ持っているか。
	int count( ItemId id ) const;

private:
	int mCounts[ ITEM_NUMBER ];
};

#endif
