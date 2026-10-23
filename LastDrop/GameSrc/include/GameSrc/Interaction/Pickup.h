#ifndef INCLUDED_PICKUP_H
#define INCLUDED_PICKUP_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Map/StaticProp.h"
#include "GameSrc/Hit/IRayBlocker.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Interaction/ItemCatalog.h"

class ViewFrustum;
class StaticMeshData;

// 地面に落ちている、拾える物1つ（ガソリンの携行缶・ゾンビが落とした弾など）。拾うとその種類のアイテムが持ち物に amount だけ増える。
// 持ちきれない分は落ちたまま残る（全部拾われたら消える）。
// 見た目と当たり判定の箱は StaticProp に任せ（委譲）、ここは「拾う」と「名簿への載り方」だけを受け持つ。
// 視線で見つけられるよう、命中の名簿（HitWorld）に「弾を止める物」として載る（見た目の三角形で当たる）。
// 移動の名簿（CollisionWorld）にも、頂点を囲む箱1つで載る（ユーザー指定。小さな物は載せないこともできる＝blocksMovement）。
// 動かない物だが、拾うと消えるので外せない「動かない箱」（addBox）ではなく、外せる相手（ICollisionBody）として載せる。
// 途中で消えるので ActorList で持つ。拾われたら isTaken が true になり、持ち主（シーン）が destroyLater で消す。
class Pickup : public IRayBlocker, public IInteractable, public IWorldMember, public ICollisionBody{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。isReady() 済みであること（箱の大きさを頂点から求めるため）。この物より長く生きていること（MeshCache が持つ）。
	// position / angleY: 置き場所（地面の上）と向き（度）。item: 拾うと増えるアイテム。amount: いくつ増えるか（1 以上）。
	// blocksMovement: 移動を止めるか。false なら移動の名簿に載らず、上を歩ける（足元の弾など、つまずくと邪魔な小さな物）。
	Pickup( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY, ItemId item, int amount, bool blocksMovement );

	// frustum: カメラに映る範囲。映らなければ描かない。
	void draw( const ViewFrustum& frustum ) const;
	// 拾われたか（拾われた物は、持ち主が消す）。
	bool isTaken() const;

	// IRayBlocker（視線で見つける。見た目の三角形で判定する）
	virtual bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;
	virtual GroundMaterial surfaceMaterial( const GameLib::Math::Vector3& point ) const; // 見た目（StaticProp）の材質
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
	StaticProp mBody; // 見た目と、頂点を囲む箱
	ItemId mItem;
	int mAmount;          // まだ落ちている数
	bool mBlocksMovement;
	bool mTaken;
};

#endif
