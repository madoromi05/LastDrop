#ifndef INCLUDED_PICKUP_COMPONENT_H
#define INCLUDED_PICKUP_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/IInteractable.h"

class ItemComponent;

// 地面に落ちている、拾える物（ガソリンの携行缶・ゾンビが落とした弾）の「拾う」部品。
// E で拾うと、同じ物の ItemComponent のアイテムを差し出し（offeredStack）、遊ぶ人がカバンの画面で選んだマスへ入った分だけ減る（takeOffered）。
// おまかせ（interact）では、入る分だけ自動で入る。持ちきれない分は落ちたまま残る（全部拾われたら isTaken）。InventoryPickup_Plan.txt Q3・Q4。
// 拾われた物は、InteractionSystem が付いている物に destroyLater で消す印を付ける。
// 見た目（StaticModelComponent。弾を止める）と箱（BoxColliderComponent）は同じ物の別の部品。
// 視線で見つけるときは、見た目の三角形ではなく、頂点を囲む箱より一回り大きい「見る箱」で当たる（lookRaycast。小さな弾薬を狙いやすくする）。
// 見る箱は名簿に載せない（弾が見えない箱で止まらないように）。探すのは InteractionSystem::findInteractable。
class PickupComponent : public Component, public IInteractable{
public:
	PickupComponent();

	// 拾われたか（拾われた物は、持ち主が消す）。
	bool isTaken() const;
	// 視線が見る箱（頂点を囲む箱を少し広げた物）に入る時刻（0〜1）を time に書いて true。
	// origin から delta だけ進む線分で調べる。拾われた物には当たらない。time: 出力先。nullptr 不可。
	bool lookRaycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, float* time ) const;

	// Component（ItemComponent・StaticModelComponent が要る）
	void start() override;
	// IInteractable
	bool canInteract( const Inventory& inventory ) const override;
	void interact( Inventory* inventory ) override;
	bool offeredStack( ItemStack* out ) const override;
	void takeOffered( int count ) override;
	ItemGrid* lootGrid() override { return nullptr; } // 拾う物は中を見せない
	float useSeconds() const override { return 0.f; } // 拾う物は待たない（カバンの画面を開く）

private:
	ItemComponent* mItem;
	// ワールド → 見る箱を -1〜1 の立方体にした空間（HitMath::segmentUnitCube で調べるため）。置いた所から動かないので start で決める。
	GameLib::Math::Matrix34 mLookBoxToLocal;
	bool mTaken;
};

#endif
