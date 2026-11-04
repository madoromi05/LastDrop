#ifndef INCLUDED_INVENTORY_H
#define INCLUDED_INVENTORY_H

#include "GameSrc/Interaction/ItemCatalog.h"
#include "GameSrc/Interaction/ItemGrid.h"

// 持ち物（カバン）。中身は 2 列のマス目（ItemGrid）。SummaryPlan/個別仕様/GridInventory_Plan.txt。
// 拾う物（Pickup）・車（FuelTank）・銃（リロード）が使う入口は canAdd / add / remove / count の4つだけにしてある。
// マス目にしても（2026-11-04）この4つは変えていないので、使う側は変わらない。
// 何マス使うか・何個重ねるかの決まりは ItemCatalog が持つ（ここには書かない）。
// カバンの画面（InventoryScreen）は grid() で中を見て、動かす・捨てる・広さを変えるはここの関数を通す。
class Inventory{
public:
	Inventory(); // 2 列 × 4 行（8 マス）。種類ごとに ItemSpec::startAmount だけ持って始まる

	// id をもう1つ持てるか（重ねられるマスか、置ける空きがある）。
	bool canAdd( ItemId id ) const;
	// id を1つ持つ。canAdd が false のときは呼ばないこと（持ちきれない分を黙って捨てないよう、ASSERT で止める）。
	void add( ItemId id );
	// id を1つ手放す（数が一番少ないマスから）。持っていないときは呼ばないこと（ASSERT で止める）。
	void remove( ItemId id );
	// id をいくつ持っているか（全部のマスの合計）。
	int count( ItemId id ) const;

	// マス目（見るだけ）。
	const ItemGrid& grid() const;
	// from の物を to を指した所へ動かせるか・動かす（決まりは ItemGrid::canMove）。
	bool canMove( int from, int to ) const;
	void move( int from, int to );
	// cell の物を全部取り出す（捨てるとき。足元に置くのは呼ぶ側）。
	ItemStack takeAll( int cell );
	// 行の数を変えられるか・変える（強化で広げる。減らすのは消える行が空のときだけ）。
	bool canSetRows( int rows ) const;
	void setRows( int rows );

private:
	ItemGrid mGrid;
};

#endif
