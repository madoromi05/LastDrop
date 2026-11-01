#ifndef INCLUDED_PICKUP_H
#define INCLUDED_PICKUP_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include "GameSrc/Map/StaticProp.h"
#include "GameSrc/Hit/IRayBlocker.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Interaction/ItemCatalog.h"

class DrawCulling;
class StaticMeshData;

// 地面に落ちている、拾える物1つ（ガソリンの携行缶・ゾンビが落とした弾など）。拾うとその種類のアイテムが持ち物に amount だけ増える。
// 持ちきれない分は落ちたまま残る（全部拾われたら消える）。
// 見た目と当たり判定の箱は StaticProp に任せ（委譲）、ここは「拾う」と「名簿への載り方」だけを受け持つ。
// 弾を止めるので、命中の名簿（HitWorld）に「弾を止める物」として載る（見た目の三角形で当たる）。
// 視線で見つけるときは、見た目の三角形ではなく、それより一回り大きい「見る箱」で当たる（lookRaycast。小さな弾薬を狙いやすくする）。
// 見る箱は名簿に載せない（弾が見えない箱で止まらないように）。探すのは GameWorld::findInteractable。
// 移動の名簿（CollisionWorld）にも、頂点を囲む箱1つで載る（ユーザー指定。小さな物は載せないこともできる＝blocksMovement）。
// 動かない物だが、拾うと消えるので外せない「動かない箱」（addBox）ではなく、外せる相手（ICollisionBody）として載せる。
// 途中で消えるので、物（GameObject。タグ Fuel・AmmoDrop）の部品として GameObjectWorld が持つ。
// 拾われたら isTaken が true になり、持ち主（GameWorld）が付いている物に destroyLater で消す印を付ける。
class Pickup : public Component, public IRayBlocker, public IInteractable, public IWorldMember, public ICollisionBody{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。isReady() 済みであること（箱の大きさを頂点から求めるため）。この物より長く生きていること（MeshCache が持つ）。
	// position / angleY: 置き場所（地面の上）と向き（度）。item: 拾うと増えるアイテム。amount: いくつ増えるか（1 以上）。
	// blocksMovement: 移動を止めるか。false なら移動の名簿に載らず、上を歩ける（足元の弾など、つまずくと邪魔な小さな物）。
	Pickup( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY, ItemId item, int amount, bool blocksMovement );

	// culling（映る範囲・描く距離・小ささ。DrawCulling）で描くと決まった時だけ描く。
	void draw( const DrawCulling& culling ) const;
	// 拾われたか（拾われた物は、持ち主が消す）。
	bool isTaken() const;

	// 視線が見る箱（頂点を囲む箱を少し広げた物）に入る時刻（0〜1）を time に書いて true。
	// origin から delta だけ進む線分で調べる。拾われた物には当たらない。time: 出力先。nullptr 不可。
	bool lookRaycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, float* time ) const;

	// IRayBlocker（弾を止める。見た目の三角形で判定する）
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
	// ワールド → 見る箱を -1〜1 の立方体にした空間（HitMath::segmentUnitCube で調べるため）。置いた所から動かないので作るときに決める。
	GameLib::Math::Matrix34 mLookBoxToLocal;
	ItemId mItem;
	int mAmount;          // まだ落ちている数
	bool mBlocksMovement;
	bool mTaken;
};

#endif
