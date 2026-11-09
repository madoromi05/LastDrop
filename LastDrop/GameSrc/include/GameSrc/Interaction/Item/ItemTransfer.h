#ifndef INCLUDED_ITEM_TRANSFER_H
#define INCLUDED_ITEM_TRANSFER_H

#include "GameSrc/Interaction/Item/ItemGrid.h"

// マス目ごとの「入れてよい物」の決まり（R2_Plan.txt 5 章）。マス目（ItemGrid）は決まりを知らないので、使う側が渡す。
enum class GridRule{
	Free,       // 決まり無し（倉庫。銃を何丁でも置ける）
	CarryLimit, // ItemSpec::carryLimit を守る（カバン。銃は 1 種類 1 丁）
	FuelOnly,   // ガソリンだけ（脱出の車の給油口）
};

// マス目の上で物を動かすときに、決まり（GridRule）を守れるかを調べてから動かす（拠点の倉庫 ⇔ カバン）。
// 動かし方（重ねる・入れ替え）は ItemGrid、守る決まりはここ、と分ける（ItemGrid を倉庫にもカバンにも使えるように）。
// 写しの上で動かしてみて、動いた後の中身が決まりを守っていれば動かせる（入れ替えで、どけた物が戻る側の決まりも調べる）。
namespace ItemTransfer{
	// grid が rule を守っているか（CarryLimit なら、どの種類も carryLimit 以下か）。
	bool satisfies( const ItemGrid& grid, GridRule rule );
	// 同じマス目の中で動かす・別のマス目へ動かす（source と target が同じなら中の並べ替え）。
	bool canMove( const ItemGrid& source, GridRule sourceRule, int from, const ItemGrid& target, GridRule targetRule, int to );
	// canMove が true のときだけ呼ぶこと。
	void move( ItemGrid* source, int from, ItemGrid* target, int to );
	// source の from の物を、別のマス目 target のおまかせの所へ動かす（漁れる場所の画面の F）。
	// ①同じ物で重ねられるマス → ②上の行・左の列の空き、の順に入れる（入れ替えはしない）。重ねて入りきらない分は次のマスへ続け、
	// それでも入らない分は source に残る。1 つでも動いたら true。source と target は別のマス目であること。
	bool quickMove( ItemGrid* source, GridRule sourceRule, int from, ItemGrid* target, GridRule targetRule );
}

#endif
