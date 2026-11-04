#ifndef INCLUDED_ITEM_GRID_H
#define INCLUDED_ITEM_GRID_H

#include "GameSrc/Interaction/ItemCatalog.h"
#include <vector>

// マス目に入った物 1 つ（同じ種類を重ねた数）。amount が 0 なら空。
struct ItemStack{
	ItemId item;
	int amount;
};

// 物を入れるマス目（列 × 行）。カバン（Inventory）が持つ。後で安全ポーチ・倉庫も同じクラスで作る（列と行の数を外から渡す）。
// マスの番号は 行 × 列の数 ＋ 列（左上が 0）。2 マスの物は横に並んだ 2 マスを使い、左のマスが中身を持つ（ownerCell）。
// 描く・入力を読むことはしない（カバンの画面 InventoryScreen の仕事）。何マス使うか・何個重ねるかは ItemSpec の表で決める。
// SummaryPlan/個別仕様/GridInventory_Plan.txt。
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

	// cell を置きたい所として指したときに、id の物の中身を持つマスになる所（2 マスの物が右にはみ出すなら左へ寄せる）。
	int placementCell( ItemId id, int cell ) const;
	// from を使っている物を、to を指した所（placementCell で寄せる）へ動かせるか。
	// 動かした先に物があれば、①同じ物で重ねられるなら足し、②それ以外は元の場所と入れ替える。入れ替えた物が入らなければ動かせない。
	bool canMove( int from, int to ) const;
	// canMove が true のときだけ呼ぶこと。
	void move( int from, int to );

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

	int mColumns;
	int mRows;
	Cells mCells;
};

#endif
