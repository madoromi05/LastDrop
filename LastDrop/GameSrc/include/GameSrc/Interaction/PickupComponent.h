#ifndef INCLUDED_PICKUP_COMPONENT_H
#define INCLUDED_PICKUP_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/IInteractable.h"

class ItemComponent;

// 地面に落ちている、拾える物（ガソリンの携行缶・弾・包帯・銃）の「拾う」部品
// E で拾うと、同じ物の ItemComponent のアイテムを差し出し（offeredStack）、遊ぶ人がカバンの画面で選んだマスへ入った分だけ減る（takeOffered）
// おまかせ（interact）では、入る分だけ自動で入る。持ちきれない分は落ちたまま残る（全部拾われたら isTaken）
// 拾われた後に物をどうするか（AfterTaken）は作るときに決まり、InteractionSystem がそれを読んで片付ける
// 見た目（StaticModelComponent。弾を止める）と箱（BoxColliderComponent）は同じ物の別の部品
// 視線で見つけるときは、見た目の三角形ではなく、頂点を囲む箱より一回り大きい「見る箱」で当たる（lookRaycast。小さな弾薬を狙いやすくする）
// 見る箱は名簿に載せない（弾が見えない箱で止まらないように）。探すのは InteractionSystem::findInteractable
class PickupComponent : public Component, public IInteractable{
public:
	// 全部拾われた後に、付いている物をどうするか
	enum class AfterTaken{
		DestroyObject,	// 物ごと消す（ガソリン・弾・包帯。落ちている間だけ在る物）
		LeaveObject,	// 物は残し、地面に置くための部品だけ外す（銃。拾った後も同じ物を手に持つ）
	};
	explicit PickupComponent( AfterTaken afterTaken );

	bool isTaken() const;			// 拾われたか（拾われた物は、InteractionSystem が片付ける）
	AfterTaken afterTaken() const;
	// 視線が見る箱（頂点を囲む箱を少し広げた物）に入る時刻（0〜1）を time に書いて true
	// origin から delta だけ進む線分で調べる。拾われた物には当たらない。time: 出力先。nullptr 不可
	bool lookRaycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, float* time ) const;

	void start() override;								// Component（ItemComponent・StaticModelComponent が要る）
	// IInteractable
	bool canInteract( const Inventory& inventory ) const override;
	void interact( Inventory* inventory ) override;
	bool offeredStack( ItemStack* out ) const override;
	void takeOffered( int count ) override;
	ItemGrid* lootGrid() override { return nullptr; }	// 拾う物は中を見せない

private:
	ItemComponent* mItem;
	// ワールド → 見る箱を -1〜1 の立方体にした空間（HitMath::segmentUnitCube で調べるため）。置いた所から動かないので start で決める
	GameLib::Math::Matrix34 mLookBoxToLocal;
	AfterTaken mAfterTaken;
	bool mTaken;
};

#endif
