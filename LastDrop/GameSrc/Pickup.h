#ifndef INCLUDED_PICKUP_H
#define INCLUDED_PICKUP_H

#include "GameLib/Math/Vector3.h"
#include "StaticMeshInstance.h"
#include "IRayBlocker.h"
#include "IInteractable.h"
#include "IWorldMember.h"
#include "ICollisionBody.h"
#include "BoxCollider.h"
#include "ItemCatalog.h"

class StaticMeshData;

// 地面に落ちている、拾える物1つ（ガソリンの携行缶など）。拾うとその種類のアイテムが持ち物に1つ増える。
// 視線で見つけられるよう、命中の名簿（HitWorld）に「弾を止める物」として載る（見た目の三角形で当たる）。
// 移動の名簿（CollisionWorld）にも、頂点を囲む箱1つで載る（ユーザー指定）。動かない物だが、拾うと消えるので
// 外せない「動かない箱」（addBox）ではなく、外せる相手（ICollisionBody）として載せる。
// 途中で消えるので ActorList で持つ。拾われたら isTaken が true になり、持ち主（シーン）が destroyLater で消す。
class Pickup : public IRayBlocker, public IInteractable, public IWorldMember, public ICollisionBody{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。isReady() 済みであること（箱の大きさを頂点から求めるため）。この物より長く生きていること（MeshCache が持つ）。
	// position / angleY: 置き場所（地面の上）と向き（度）。item: 拾うと増えるアイテム。
	Pickup( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY, ItemId item );

	void draw() const;
	// 拾われたか（拾われた物は、持ち主が消す）。
	bool isTaken() const;

	// IRayBlocker（視線で見つける。見た目の三角形で判定する）
	virtual bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;
	// IInteractable
	virtual bool canInteract( const Inventory& inventory ) const;
	virtual std::string prompt( const Inventory& inventory ) const;
	virtual void interact( Inventory* inventory );
	// ICollisionBody（置いた所から動かないので、作ったときの箱をそのまま返す）
	virtual BoxCollider collider() const;
	// IWorldMember（移動と命中の両方の名簿に載る）
	virtual void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld );
	virtual void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld );

private:
	StaticMeshInstance mMesh;
	BoxCollider mCollider;
	ItemId mItem;
	bool mTaken;
};

#endif
