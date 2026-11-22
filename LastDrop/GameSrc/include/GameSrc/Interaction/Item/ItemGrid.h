#ifndef INCLUDED_ITEM_GRID_H
#define INCLUDED_ITEM_GRID_H

#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include <vector>

// マス目に入った物 1 つ（同じ種類を重ねた数）。amount が 0 なら空
struct ItemStack{
	ItemId item;
	int amount;
	// 銃に込めた弾の数（銃だけ。ほかの物は 0）。拠点の倉庫・カバンで覚え、出撃の始めに銃の弾倉へ入れる
	// 町の中では銃の物（GunComponent）が本当の数を持ち、ここは脱出したときに書き戻すまで古いまま。マスを動かすと一緒に動く
	int loadedRounds = 0;
};

// 物を入れるマス目（列 × 行）。カバン（Inventory）・漁れる場所・給油口が持つ（列と行の数を外から渡す）
// 別のマス目へ動かす（倉庫 ⇔ カバン）のは moveBetween。「カバンに銃は 1 丁」などの入れてよい物の決まりは知らない（ItemTransfer が持つ）
// マスの番号は 行 × 列の数 ＋ 列（左上が 0）。2 マスの物は横に並んだ 2 マスを使い、左のマスが中身を持つ（ownerCell）
// 描く・入力を読むことはしない（カバンの画面 InventoryScreen の仕事）。何マス使うか・何個重ねるかは ItemSpec の表で決める
// SummaryPlan/個別仕様/出撃と帰還/GridInventory_Plan.txt
class ItemGrid{
public:
	ItemGrid( int columns, int rows );	// columns / rows: 1 以上

	int columns() const;
	int rows() const;
	int cellCount() const;

	int ownerCell( int cell ) const;				// cell を使っている物の、中身を持つマス（2 マスの物の右のマスなら左のマス）。空のマスなら -1
	const ItemStack& stackAt( int owner ) const;	// 中身を持つマス owner の物。owner は ownerCell( owner ) == owner であること
	int total( ItemId id ) const;					// id を全部のマスで合わせていくつ持っているか

	bool canAddOne( ItemId id ) const;				// id をもう 1 つ入れられるか（重ねられるマスか、置ける空きがある）
	void addOne( ItemId id );						// id を 1 つ入れる。①同じ物で重ねられるマスへ足す → ②上の行・左の列から、入る空きへ置く。canAddOne が false なら呼ばないこと
	// id を 1 つ減らす。数が一番少ないマス（同じなら下のマス）から減らす。弾を使うと半端なマスから空くように
	// 持っていないなら呼ばないこと
	void removeOne( ItemId id );
	ItemStack takeAll( int cell );					// cell を使っている物を、重ねた分まで全部取り出す（マスは空く）。空のマスなら呼ばないこと
	// stack をまるごと owner のマスへ置けるか・置く（保存のファイルから読むとき。重ねず、そのマスと右のマスが空いていること）
	// 置けなければ何もせず false。stack.amount は 1〜stackSize であること（ASSERT で止める）
	bool placeAt( int owner, const ItemStack& stack );
	bool placeAnywhere( const ItemStack& stack );	// stack をまるごと、上の行・左の列から最初に空いている所へ置く（始めの倉庫・カバンを作るとき）。置けなければ何もせず false
	// 拾った物 stack を、cell を指した所（placementCell で寄せる）へ入れたら何個入るか
	// 入るのは、空いている所（1 マスに重ねられる数まで）か、同じ物で重ねられるマス（空きの分まで）だけ。物があるマスは 0（入れ替えはしない）
	int countPlaceableInto( int cell, const ItemStack& stack ) const;
	// stack を cell を指した所へ count 個入れる。count は 1〜countPlaceableInto であること（ASSERT で止める）
	// 空いている所へ置くときは込めた弾の数も写す（銃）
	void placeInto( int cell, const ItemStack& stack, int count );
	void setLoadedRounds( int owner, int rounds );	// 中身を持つマス owner の銃に込めた弾の数を変える（脱出したときに、銃の物の弾倉の数を書き戻す）
	std::vector< ItemStack > stacks() const;		// 中身を持つマスの物を、上のマスから順に全部（数と込めた弾ごと）。結果の画面・スコア・保存に使う
	void clearAll();								// 全部のマスを空にする（ゲームオーバーで 1 日目の形に戻すとき）

	int placementCell( ItemId id, int cell ) const;	// cell を置きたい所として指したときに、id の物の中身を持つマスになる所（2 マスの物が右にはみ出すなら左へ寄せる）
	// from を使っている物を、to を指した所（placementCell で寄せる）へ動かせるか
	// 動かした先に物があれば、①同じ物で重ねられるなら足し、②それ以外は元の場所と入れ替える。入れ替えた物が入らなければ動かせない
	bool canMove( int from, int to ) const;
	void move( int from, int to );					// canMove が true のときだけ呼ぶこと
	// source の from を使っている物を、別のマス目 target の to を指した所へ動かせるか・動かす（倉庫 ⇔ カバン）。決まりは canMove と同じ
	// （重ねられるなら足す・それ以外は入れ替え。入れ替えた物は source の元の場所と同じ並びへ、入らなければ source の空いている所へ）
	// source と target は別のマス目であること（同じなら canMove / move を使う）。moveBetween は canMoveBetween が true のときだけ呼ぶこと
	static bool canMoveBetween( const ItemGrid& source, int from, const ItemGrid& target, int to );
	static void moveBetween( ItemGrid* source, int from, ItemGrid* target, int to );

	bool canSetRows( int rows ) const;	// 行の数を rows にできるか（増やすのはいつでも。減らすのは、消える行に物が無いときだけ）
	void setRows( int rows );			// canSetRows が true のときだけ呼ぶこと

private:
	// マス 1 つ。owner は中身を持つマスの番号（空なら -1）。stack は owner が自分のマスのときだけ意味がある
	struct Cell{
		int owner;
		ItemStack stack;
	};
	using Cells = std::vector< Cell >;

	bool isInside( int row, int column, int size ) const;
	bool isFree( const Cells& cells, int owner, int size ) const;
	int firstFree( const Cells& cells, int size ) const;
	static void place( Cells* cells, int owner, const ItemStack& stack );
	static void clear( Cells* cells, int owner );
	bool applyMove( Cells* cells, int from, int to ) const;																// cells の上で move をしてみる。できたら true（cells は動かした後）。できなければ false（cells は途中まで変わっている）
	bool applyMoveBetween( Cells* sourceCells, int from, const ItemGrid& target, Cells* targetCells, int to ) const;	// this のマス目（の写し sourceCells）の from の物を、target のマス目（の写し targetCells）の to へ動かしてみる。決まりと答えは applyMove と同じ
	static std::vector< int > ownersIn( const Cells& cells, int owner, int size, int except );							// cells の owner から size マスぶん（横）を使っている、except とは別の物の中身を持つマス（重ならないように並べる）

	int mColumns;
	int mRows;
	Cells mCells;
};

#endif
