#ifndef INCLUDED_LOOT_CONTAINER_COMPONENT_H
#define INCLUDED_LOOT_CONTAINER_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Item/ItemGrid.h"
#include "GameSrc/Interaction/Loot/LootTable.h"
#include "GameLib/Math/Random.h"
#include <vector>

// 漁れる場所の部品（ゴミ箱・木箱。SummaryPlan/個別仕様/出撃と帰還/R3_Plan.txt 5 章）
// 中身のマス目（2 × 3）と、どの表の場所か・もう開けたか、を持つ。どの物に付けても同じ部品（違うのは表の種類だけ）
// 見た目と箱は同じ物の別の部品（ゴミ箱・木箱は StaticModelComponent）
// 開けるまで待つのは LootSearch、中を見る画面は LootScreen（どちらも GameScene が持つ）。ここは中身と「開けたか」だけ
// 中身はその日のうちだけ（置いていった物も残る）。保存はしない
class LootContainerComponent : public Component, public IInteractable{
public:
	// table: どの表から中身を決めるか。random: 中身を決める乱数（作るときに 1 回だけ使う。覚えない。nullptr 不可）
	// guaranteed: 必ず入れる物（町のガソリン・ライフル。空なら無し。中身を振り直しても入れ直す）
	LootContainerComponent( LootTableId table, GameLib::Math::Random* random, const std::vector< ItemStack >& guaranteed );

	LootTableId table() const;
	// もう開けたか（開けた所は、次からは待たずにすぐ開く）
	bool isSearched() const;
	// 開け終わった（LootSearch が待ち終えた・待たずに開いた）
	void markSearched();
	// 開けるまでにかかる秒（開けた所は 0）
	float searchSeconds() const;
	// 中身（見るだけ）
	const ItemGrid& contents() const;
	// 中身を表から決め直し、まだ開けていない形に戻す（F6 の確認用。random: nullptr 不可）
	void reroll( GameLib::Math::Random* random );

	// IInteractable
	// 開けていない所はいつも使える。開けた所は、中身が残っている間だけ（空なら灰色のマーク）
	bool canInteract( const Inventory& inventory ) const override;
	void interact( Inventory* inventory ) override;     // 呼ばれない（Interactor は lootGrid のある物を使わずに opened へ回す）
	bool offeredStack( ItemStack* out ) const override; // 拾う物ではないので false
	void takeOffered( int count ) override;             // 呼ばれない
	ItemGrid* lootGrid() override;

private:
	LootTableId mTable;
	std::vector< ItemStack > mGuaranteed;
	ItemGrid mContents;
	bool mSearched;
};

#endif
