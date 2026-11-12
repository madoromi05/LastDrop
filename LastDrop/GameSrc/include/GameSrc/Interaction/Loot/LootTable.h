#ifndef INCLUDED_LOOT_TABLE_H
#define INCLUDED_LOOT_TABLE_H

#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Audio/SoundCatalog.h"
#include "GameLib/Math/Random.h"
#include <vector>

// 漁れる場所の種類（SummaryPlan/個別仕様/出撃と帰還/R3_Plan.txt Q1）。種類ごとに、出やすい物・開けるのにかかる秒・開けている音が違う
// 見た目（ゴミ箱・木箱の形）は種類と関係なく PropFactory が決める。ゾンビの死体の表は無い（死体は漁れない）
enum class LootTableId{
	Dumpster, // ゴミ箱（9mm・包帯・腕時計）
	Crate,    // 木箱（弾・包帯。町のライフルは必ず入れる物として 1 つの木箱へ）
	Count,
};

// 場所ごとの「出る物・重み・数の幅」の表（データは LootTable.cpp）と、表から中身を決める関数。状態は持たない
// 表に 1 行足せば出る物が増える（ItemCatalog と同じ。roll の処理は変えない）
// 開ける秒・空になる確率は F6 で動かすので LootParams にある（ここはそれを種類で引くだけ）
namespace LootTable{
	// 場所の中身のマス目の大きさ（カバンと同じ 2 列なので、2 マスの物もそのまま入る）
	const int kColumns = 2;
	const int kRows = 3;

	// 画面に出す名前（中を見る画面の見出し。日本語）
	const char* displayName( LootTableId id );
	// 開けるのにかかる秒（LootParams。0 ならすぐ開く）
	float searchSeconds( LootTableId id );
	// 開けている間に鳴らす音（ループ。金属・木）
	SoundId searchSound( LootTableId id );
	// 表から中身を決めて out へおまかせで入れる（out は先に空にする）。空になることもある（LootParams の確率）
	// 入りきらない物は出さない。random: 呼ぶ側が種を決めた乱数（同じ種なら同じ中身。nullptr 不可）。out: nullptr 不可
	// guaranteed: 必ず入れる物（空なら無し。町のガソリン・ライフル）。先に入れてから残りのマスに振り、
	// 入れた場所は空にならない（空の箱にガソリンだけ、にはしない）
	void roll( LootTableId id, GameLib::Math::Random* random, const std::vector< ItemStack >& guaranteed, ItemGrid* out );
}

#endif
