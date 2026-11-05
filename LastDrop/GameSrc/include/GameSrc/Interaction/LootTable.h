#ifndef INCLUDED_LOOT_TABLE_H
#define INCLUDED_LOOT_TABLE_H

#include "GameSrc/Interaction/ItemGrid.h"
#include "GameSrc/Audio/SoundCatalog.h"
#include "GameLib/Math/Random.h"

// 漁れる場所の種類（SummaryPlan/個別仕様/R3_Plan.txt Q1。車のトランクは FuelEscapeLoading_Plan.txt 改訂 2 で消した）。種類ごとに、出やすい物・開けるのにかかる秒・開けている音が違う。
// 見た目（車・ゴミ箱・木箱の形、死体）は種類と関係なく PropFactory・CombatResolver が決める。
enum class LootTableId{
	Dumpster, // ゴミ箱（9mm・包帯・腕時計）
	Crate,    // 木箱（弾・ライフル）
	Corpse,   // ゾンビの死体（弾。たまに包帯・腕時計）
	Count,
};

// 場所ごとの「出る物・重み・数の幅」の表（データは LootTable.cpp）と、表から中身を決める関数。状態は持たない。
// 表に 1 行足せば出る物が増える（ItemCatalog と同じ。roll の処理は変えない）。
// 開ける秒・空になる確率は F6 で動かすので LootParams にある（ここはそれを種類で引くだけ）。
namespace LootTable{
	// 場所の中身のマス目の大きさ（R3_Plan.txt Q9。カバンと同じ 2 列なので、2 マスの物もそのまま入る）。
	const int kColumns = 2;
	const int kRows = 3;

	// 画面に出す名前（中を見る画面の見出し。日本語）。
	const char* displayName( LootTableId id );
	// 開けるのにかかる秒（LootParams。0 ならすぐ開く）。
	float searchSeconds( LootTableId id );
	// 開けている間に鳴らす音（ループ。金属・木・布）。
	SoundId searchSound( LootTableId id );
	// 表から中身を決めて out へおまかせで入れる（out は先に空にする）。空になることもある（LootParams の確率）。
	// 入りきらない物は出さない。random: 呼ぶ側が種を決めた乱数（同じ種なら同じ中身。nullptr 不可）。out: nullptr 不可。
	// guaranteed: 必ず入れる物（ITEM_NONE なら無し。町のガソリン。FuelEscapeLoading_Plan.txt Q15）。先に 1 個入れてから残りのマスに振り、
	// 入れた場所は空にならない（空の箱にガソリンだけ、にはしない）。
	void roll( LootTableId id, GameLib::Math::Random* random, ItemId guaranteed, ItemGrid* out );
}

#endif
