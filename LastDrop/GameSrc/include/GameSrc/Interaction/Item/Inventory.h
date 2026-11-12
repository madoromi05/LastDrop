#ifndef INCLUDED_INVENTORY_H
#define INCLUDED_INVENTORY_H

#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"

// 持ち物（カバン）。中身は 2 列のマス目（ItemGrid）。SummaryPlan/個別仕様/出撃と帰還/GridInventory_Plan.txt
// 拾う物（Pickup）・銃（リロード）が使う入口は canAdd / add / remove / count の4つだけにしてある
// マス目にしてもこの4つは変えていないので、使う側は変わらない
// 何マス使うか・何個重ねるかの決まりは ItemCatalog が持つ（ここには書かない）
// カバンの画面（InventoryScreen）は grid() で中を見て、動かす・捨てる・広さを変えるはここの関数を通す
class Inventory{
public:
	explicit Inventory( const ItemGrid& bag );	// bag: 始めのカバンの中身（RaidStart::startBag。写して持つ）。広さも bag のまま

	// id をもう1つ持てるか（重ねられるマスか、置ける空きがあり、カバンに何個までの決まり ItemSpec::carryLimit を越えない）
	// 2 丁目の銃は拾えない（物の上のマークが灰色になる）
	bool canAdd( ItemId id ) const;
	void add( ItemId id );			// id を1つ持つ。canAdd が false のときは呼ばないこと（持ちきれない分を黙って捨てないよう、ASSERT で止める）
	void remove( ItemId id );		// id を1つ手放す（数が一番少ないマスから）。持っていないときは呼ばないこと（ASSERT で止める）
	int count( ItemId id ) const;	// id をいくつ持っているか（全部のマスの合計）

	const ItemGrid& grid() const;					// マス目（見るだけ）
	// 別のマス目との間で動かすためのマス目（漁れる場所の画面だけが使う）
	// カバンに何個までの決まり（carryLimit）は、動かす側が ItemTransfer に GridRule::CarryLimit を渡して守る
	ItemGrid* gridForTransfer();
	// from の物を to を指した所へ動かせるか・動かす（決まりは ItemGrid::canMove）
	bool canMove( int from, int to ) const;
	void move( int from, int to );
	ItemStack takeAll( int cell );					// cell の物を全部取り出す（捨てるとき。足元に置くのは呼ぶ側）
	// 行の数を変えられるか・変える（強化で広げる。減らすのは消える行が空のときだけ）
	bool canSetRows( int rows ) const;
	void setRows( int rows );
	// 拾った物 stack を、cell を指した所へ何個入れられるか・入れる（置く所を選んで拾う）
	// 決まりは ItemGrid::countPlaceableInto に、カバンに何個までの決まり（ItemSpec::carryLimit）を足した物
	int countPlaceableInto( int cell, const ItemStack& stack ) const;
	void placeInto( int cell, const ItemStack& stack, int count );
	void setLoadedRounds( int owner, int rounds );	// 中身を持つマス owner の銃に込めた弾の数を書く（脱出したときに、銃の物の弾倉の数を写す）

private:
	ItemGrid mGrid;
};

#endif
