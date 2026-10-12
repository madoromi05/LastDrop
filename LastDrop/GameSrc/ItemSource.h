#ifndef INCLUDED_ITEM_SOURCE_H
#define INCLUDED_ITEM_SOURCE_H

#include "GameLib/Math/Vector3.h"
#include "StaticProp.h"
#include "IInteractable.h"
#include "ItemCatalog.h"

class ViewFrustum;
class StaticMeshData;
class CollisionWorld;
class HitWorld;

// 何度でも拾える物の置き場（車の横のガソリン缶など）。見て操作キーで、その種類のアイテムが持ち物に1つ増える。拾っても消えない。
// 見た目・移動の当たり判定（箱1つ）・視線の当たり判定は StaticProp に任せ、ここは「拾わせる」だけを受け持つ。
// Pickup と分けている: Pickup は拾うと消える物で、消すための仕組み（ActorList・名簿から外す）を持つ。
// こちらは最後まで置いておく物なので、車（EscapeCar）と同じく名簿へ載せたままにする。
// 一度に持てる数は変えない（持ち物の上限は Inventory が決める）。
class ItemSource : public IInteractable{
public:
	// data: 形と見た目（StaticProp と同じ約束）。position / angleY: 置き場所と向き（度）。item: 拾うと増えるアイテム。
	ItemSource( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY, ItemId item );

	// 移動の名簿へ箱を、命中の名簿へ「弾（視線）を止める物」として自分の見た目を載せる。
	// この置き場はどちらの名簿よりも先に消えないこと（名簿は借りるだけなので、外す処理を持たない。シーンの最後まで置いておく物）。
	void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) const;
	// frustum: カメラに映る範囲。映らなければ描かない。
	void draw( const ViewFrustum& frustum ) const;

	// 視線が当たった物がこの置き場か（Interactor が、当たった物から使える物を探すため）。
	bool isBody( const IRayBlocker* blocker ) const;

	// IInteractable
	virtual bool canInteract( const Inventory& inventory ) const;
	virtual std::string prompt( const Inventory& inventory ) const;
	virtual void interact( Inventory* inventory );

private:
	StaticProp mBody;
	ItemId mItem;
};

#endif
