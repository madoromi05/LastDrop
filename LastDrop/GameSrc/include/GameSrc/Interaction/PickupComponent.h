#ifndef INCLUDED_PICKUP_COMPONENT_H
#define INCLUDED_PICKUP_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Interaction/IInteractable.h"

class ItemComponent;

// 地面に落ちている、拾える物（ガソリンの携行缶・ゾンビが落とした弾）の「拾う」部品。
// 拾うと、同じ物の ItemComponent のアイテムが持ち物に増える。持ちきれない分は落ちたまま残る（全部拾われたら isTaken）。
// 拾われた物は、持ち主（GameWorld）が付いている物に destroyLater で消す印を付ける。
// 見た目（StaticModelComponent。弾を止める）と箱（BoxColliderComponent）は同じ物の別の部品。
// 視線で見つけるときは、見た目の三角形ではなく、箱より一回り大きい「見る箱」で当たる（lookRaycast。小さな弾薬を狙いやすくする）。
// 見る箱は名簿に載せない（弾が見えない箱で止まらないように）。探すのは GameWorld::findInteractable。
class PickupComponent : public Component, public IInteractable{
public:
	PickupComponent();

	// 拾われたか（拾われた物は、持ち主が消す）。
	bool isTaken() const;
	// 視線が見る箱（頂点を囲む箱を少し広げた物）に入る時刻（0〜1）を time に書いて true。
	// origin から delta だけ進む線分で調べる。拾われた物には当たらない。time: 出力先。nullptr 不可。
	bool lookRaycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, float* time ) const;

	// Component（ItemComponent・BoxColliderComponent が要る）
	virtual void start();
	// IInteractable
	virtual bool canInteract( const Inventory& inventory ) const;
	virtual std::string prompt( const Inventory& inventory ) const;
	virtual void interact( Inventory* inventory );

private:
	ItemComponent* mItem;
	// ワールド → 見る箱を -1〜1 の立方体にした空間（HitMath::segmentUnitCube で調べるため）。置いた所から動かないので start で決める。
	GameLib::Math::Matrix34 mLookBoxToLocal;
	bool mTaken;
};

#endif
