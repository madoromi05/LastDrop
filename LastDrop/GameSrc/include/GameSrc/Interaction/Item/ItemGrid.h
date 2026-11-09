#ifndef INCLUDED_ITEM_GRID_H
#define INCLUDED_ITEM_GRID_H

#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include <vector>

// マス目に入った物 1 つ（同じ種類を重ねた数）。amount が 0 なら空。
struct ItemStack{
	ItemId item;
	int amount;
	// 銃に込めた弾の数（銃だけ。ほかの物は 0）。拠点の倉庫・カバンで覚え、出撃の始めに銃の弾倉へ入れる（R2_Plan.txt Q13）。
	// 町の中では銃の物（GunComponent）が本当の数を持ち、ここは脱出したときに書き戻すまで古いまま。マスを動かすと一緒に動く。
	int loadedRounds = 0;
};

// 物を入れるマス目（列 × 行）。カバン（Inventory）・漁れる場所・給油口が持つ（列と行の数を外から渡す。倉庫の画面 StashScreen 用のコードも残してある）。
// 別のマス目へ動かす（倉庫 ⇔ カバン）のは moveBetween。「カバンに銃は 1 丁」などの入れてよい物の決まりは知らない（ItemTransfer が持つ）。
// マスの番号は 行 × 列の数 ＋ 列（左上が 0）。2 マスの物は横に並んだ 2 マスを使い、左のマスが中身を持つ（ownerCell）。
// 描く・入力を読むことはしない（カバンの画面 InventoryScreen の仕事）。何マス使うか・何個重ねるかは ItemSpec の表で決める。
// SummaryPlan/個別仕様/出撃と帰還/GridInventory_Plan.txt。
class ItemGrid{
public:
	// columns / rows: 1 以上。
	ItemGrid( int columns, int rows );

	int columns() const;
	int rows() const;
	int cellCount() const;

	// cell を使っている物の、中身を持つマス（2 マスの物の右のマスなら左のマス）。空のマスなら -1。
	int ownerCell( int cell ) const;
	// 中身を持つマス owner の物。owner は ownerCell( owner ) == owner であること。
	const ItemStack& stackAt( int owner ) const;
	// id を全部のマスで合わせていくつ持っているか。
	int total( ItemId id ) const;

	// id をもう 1 つ入れられるか（重ねられるマスか、置ける空きがある）。
	bool canAddOne( ItemId id ) const;
	// id を 1 つ入れる。①同じ物で重ねられるマスへ足す → ②上の行・左の列から、入る空きへ置く。canAddOne が false なら呼ばないこと。
	void addOne( ItemId id );
	// id を 1 つ減らす。数が一番少ないマス（同じなら下のマス）から減らす。弾を使うと半端なマスから空くように。
	// 持っていないなら呼ばないこと。
	void removeOne( ItemId id );
	// cell を使っている物を、重ねた分まで全部取り出す（マスは空く）。空のマスなら呼ばないこと。
	ItemStack takeAll( int cell );
	// stack をまるごと owner のマスへ置けるか・置く（保存のファイルから読むとき。重ねず、そのマスと右のマスが空いていること）。
	// 置けなければ何もせず false。stack.amount は 1〜stackSize であること（ASSERT で止める）。
	bool placeAt( int owner, const ItemStack& stack );
	// stack をまるごと、上の行・左の列から最初に空いている所へ置く（始めの倉庫・カバンを作るとき）。置けなければ何もせず false。
	bool placeAnywhere( const ItemStack& stack );
	// 拾った物 stack を、cell を指した所（placementCell で寄せる）へ入れたら何個入るか（InventoryPickup_Plan.txt Q3）。
	// 入るのは、空いている所（1 マスに重ねられる数まで）か、同じ物で重ねられるマス（空きの分まで）だけ。物があるマスは 0（入れ替えはしない）。
	int countPlaceableInto( int cell, const ItemStack& stack ) const;
	// stack を cell を指した所へ count 個入れる。count は 1〜countPlaceableInto であること（ASSERT で止める）。
	// 空いている所へ置くときは込めた弾の数も写す（銃）。
	void placeInto( int cell, const ItemStack& stack, int count );
	// 中身を持つマス owner の銃に込めた弾の数を変える（脱出したときに、銃の物の弾倉の数を書き戻す）。
	void setLoadedRounds( int owner, int rounds );
	// 中身を持つマスの物を、上のマスから順に全部（数と込めた弾ごと）。結果の画面・スコア・保存に使う。
	std::vector< ItemStack > stacks() const;
	// 全部のマスを空にする（ゲームオーバーで 1 日目の形に戻すとき）。
	void clearAll();

	// cell を置きたい所として指したときに、id の物の中身を持つマスになる所（2 マスの物が右にはみ出すなら左へ寄せる）。
	int placementCell( ItemId id, int cell ) const;
	// from を使っている物を、to を指した所（placementCell で寄せる）へ動かせるか。
	// 動かした先に物があれば、①同じ物で重ねられるなら足し、②それ以外は元の場所と入れ替える。入れ替えた物が入らなければ動かせない。
	bool canMove( int from, int to ) const;
	// canMove が true のときだけ呼ぶこと。
	void move( int from, int to );
	// source の from を使っている物を、別のマス目 target の to を指した所へ動かせるか・動かす（倉庫 ⇔ カバン）。決まりは canMove と同じ
	// （重ねられるなら足す・それ以外は入れ替え。入れ替えた物は source の元の場所と同じ並びへ、入らなければ source の空いている所へ）。
	// source と target は別のマス目であること（同じなら canMove / move を使う）。moveBetween は canMoveBetween が true のときだけ呼ぶこと。
	static bool canMoveBetween( const ItemGrid& source, int from, const ItemGrid& target, int to );
	static void moveBetween( ItemGrid* source, int from, ItemGrid* target, int to );

	// 行の数を rows にできるか（増やすのはいつでも。減らすのは、消える行に物が無いときだけ）。
	bool canSetRows( int rows ) const;
	// canSetRows が true のときだけ呼ぶこと。
	void setRows( int rows );

private:
	// マス 1 つ。owner は中身を持つマスの番号（空なら -1）。stack は owner が自分のマスのときだけ意味がある。
	struct Cell{
		int owner;
		ItemStack stack;
	};
	typedef std::vector< Cell > Cells;

	bool isInside( int row, int column, int size ) const;
	bool isFree( const Cells& cells, int owner, int size ) const;
	int firstFree( const Cells& cells, int size ) const;
	static void place( Cells* cells, int owner, const ItemStack& stack );
	static void clear( Cells* cells, int owner );
	// cells の上で move をしてみる。できたら true（cells は動かした後）。できなければ false（cells は途中まで変わっている）。
	bool applyMove( Cells* cells, int from, int to ) const;
	// this のマス目（の写し sourceCells）の from の物を、target のマス目（の写し targetCells）の to へ動かしてみる。決まりと答えは applyMove と同じ。
	bool applyMoveBetween( Cells* sourceCells, int from, const ItemGrid& target, Cells* targetCells, int to ) const;
	// cells の owner から size マスぶん（横）を使っている、except とは別の物の中身を持つマス（重ならないように並べる）。
	static std::vector< int > ownersIn( const Cells& cells, int owner, int size, int except );

	int mColumns;
	int mRows;
	Cells mCells;
};

#endif
