#ifndef INCLUDED_BOX_COLLIDER_COMPONENT_H
#define INCLUDED_BOX_COLLIDER_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/BoxCollider.h"

// 置物の移動の当たり判定（頂点を真上から囲む箱 1 つ。前の StaticProp の箱の側）。
// 箱は同じ物の StaticModelComponent の頂点から start で求める（見た目は awake で置き場所へ動いている。置いた後は動かない）。
// 移動の名簿（CollisionWorld）への載り方は 3 通り（Mode）。どれにするかは物を組み立てる側（PropFactory など）が決める。
class BoxColliderComponent : public Component, public ICollisionBody, public IWorldMember{
public:
	enum class Mode{
		Fixed,       // 動かない箱として載る（車・何度でも拾える置き場。最後まで置いておく物。名簿の方が値を写して持ち、外せない）
		Removable,   // 外せる相手として載る（拾うと消えるガソリン）
		WalkThrough, // 載らない（足元の弾。上を歩ける。箱は視線で探す見る箱にだけ使う）
	};
	explicit BoxColliderComponent( Mode mode );

	// Component（StaticModelComponent が要る）
	virtual void start();
	// ICollisionBody（置いた所から動かないので、start で求めた箱をそのまま返す）
	virtual BoxCollider collider() const;
	// IWorldMember（移動の名簿にだけ載る。Fixed は外せないので removeFrom は何もしない）
	virtual void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld );
	virtual void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld );

private:
	Mode mMode;
	BoxCollider mBox;
};

#endif
