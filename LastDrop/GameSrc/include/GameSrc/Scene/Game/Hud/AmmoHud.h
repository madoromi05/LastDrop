#ifndef INCLUDED_AMMO_HUD_H
#define INCLUDED_AMMO_HUD_H

#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameSrc/Scene/Game/Screen/ItemIcons.h"

class UiRenderer;

// 右下の、銃の絵と「弾倉の弾の数 / 予備の弾の数」（SummaryPlan/個別仕様/UI/AmmoHud_Plan.txt）
// 半透明の黒い板の上に、左に銃の絵（カバンと同じ絵）、右に数を出す。弾倉が満タンなら弾倉の数を緑、空なら赤にする
// 表示するだけで、銃・カバンは読まない（呼ぶ側が数を渡す）。いつ出すか・濃さは AmmoHudVisibility が決める
// 積むだけで UiRenderer::draw は呼ばない（UiRenderer は 1 回の描画に 1 回だけ。RaidHud がまとめて描く）
// 並べ方・色を変えるときは AmmoHud.cpp の定数だけを直す
class AmmoHud{
public:
	// 出す中身
	struct Ammo{
		ItemId weapon;   // 手に持っている銃の物（絵を引く）
		int rounds;      // 弾倉に入っている弾の数
		int capacity;    // 弾倉に入る弾の数（満タンかを見る）
		int reserve;     // カバンにある、銃に合った弾の数
	};

	// renderer: 借りる。所有権は受け取らない。nullptr 不可。この HUD より長く生きていること
	explicit AmmoHud( UiRenderer* renderer );
	AmmoHud( const AmmoHud& ) = delete;
	AmmoHud& operator=( const AmmoHud& ) = delete;

	void add( const Ammo& ammo, float opacity );	// 右下に積む。opacity: 濃さ（0〜1。AmmoHudVisibility::opacity）。0 なら何も積まない

private:
	UiRenderer* mRenderer; // 借りているだけ
	ItemIcons mIcons;      // 銃の絵（作った時に読み込みを始める）
};

#endif
