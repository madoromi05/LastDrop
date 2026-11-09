#ifndef INCLUDED_INVENTORY_H
#define INCLUDED_INVENTORY_H

#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"

// 持ち物（カバン）。中身は 2 列のマス目（ItemGrid）。SummaryPlan/個別仕様/出撃と帰還/GridInventory_Plan.txt。
// 拾う物（Pickup）・銃（リロード）が使う入口は canAdd / add / remove / count の4つだけにしてある。
// マス目にしても（2026-11-04）この4つは変えていないので、使う側は変わらない。
// 何マス使うか・何個重ねるかの決まりは ItemCatalog が持つ（ここには書かない）。
// カバンの画面（InventoryScreen）は grid() で中を見て、動かす・捨てる・広さを変えるはここの関数を通す。
class Inventory{
public:
	// bag: 始めのカバンの中身（RaidStart::startBag。写して持つ。SimpleRun_Plan.txt Q6）。広さも bag のまま。
	explicit Inventory( const ItemGrid& bag );

	// id をもう1つ持てるか（重ねられるマスか、置ける空きがあり、カバンに何個までの決まり ItemSpec::carryLimit を越えない）。
	// 2 丁目の銃は拾えない（物の上のマークが灰色になる。文字の案内は 11-04 にマークへ替えたので出さない）。
	bool canAdd( ItemId id ) const;
	// id を1つ持つ。canAdd が false のときは呼ばないこと（持ちきれない分を黙って捨てないよう、ASSERT で止める）。
	void add( ItemId id );
	// id を1つ手放す（数が一番少ないマスから）。持っていないときは呼ばないこと（ASSERT で止める）。
	void remove( ItemId id );
	// id をいくつ持っているか（全部のマスの合計）。
	int count( ItemId id ) const;

	// マス目（見るだけ）。
	const ItemGrid& grid() const;
	// 別のマス目との間で動かすためのマス目（漁れる場所の画面だけが使う。R3_Plan.txt Q9）。
	// カバンに何個までの決まり（carryLimit）は、動かす側が ItemTransfer に GridRule::CarryLimit を渡して守る。
	ItemGrid* gridForTransfer();
	// from の物を to を指した所へ動かせるか・動かす（決まりは ItemGrid::canMove）。
	bool canMove( int from, int to ) const;
	void move( int from, int to );
	// cell の物を全部取り出す（捨てるとき。足元に置くのは呼ぶ側）。
	ItemStack takeAll( int cell );
	// 行の数を変えられるか・変える（強化で広げる。減らすのは消える行が空のときだけ）。
	bool canSetRows( int rows ) const;
	void setRows( int rows );
	// 拾った物 stack を、cell を指した所へ何個入れられるか・入れる（置く所を選んで拾う。InventoryPickup_Plan.txt Q3）。
	// 決まりは ItemGrid::countPlaceableInto に、カバンに何個までの決まり（ItemSpec::carryLimit）を足した物。
	int countPlaceableInto( int cell, const ItemStack& stack ) const;
	void placeInto( int cell, const ItemStack& stack, int count );
	// 中身を持つマス owner の銃に込めた弾の数を書く（脱出したときに、銃の物の弾倉の数を写す。R2_Plan.txt Q13）。
	void setLoadedRounds( int owner, int rounds );

private:
	ItemGrid mGrid;
};

#endif
