#ifndef INCLUDED_CHARACTER_COLLIDER_COMPONENT_H
#define INCLUDED_CHARACTER_COLLIDER_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/CharacterMover.h"

class CollisionWorld;

// キャラの移動の当たり判定（体を真上から囲む長方形）と、壁・他のキャラに沿って滑る移動（CharacterMover。Collide-and-Slide）
// 自キャラ・ゾンビが持つ。移動の名簿（CollisionWorld）へ自分で載り、他のキャラの移動を止める（IWorldMember）
// 長方形の形はキャラごとに違う（自キャラは目と銃の先まで伸ばす、ゾンビは体を引きずる間は胸に置く）ので、
// 形を知っている部品（PlayerComponent・ZombieStateComponent）へその都度聞く。名簿に載るのはこの部品の方
// 形を知っている部品は、自分の start で setShape する
// 滑る移動（mover）はこの部品と一緒に作られるので、借りる部品（ZombieChaseComponent など）は awake から借りてよい
// （動かせるのは、この部品の awake で名簿を渡した後＝update から）
// 倒れたゾンビの上を歩けるように、途中で名簿から外せる（stopBlocking。外しても、自分が滑って動くのには使える）
class CharacterColliderComponent : public Component, public ICollisionBody, public IWorldMember{
public:
	CharacterColliderComponent();

	// 長方形の形を答える部品（同じ物の部品）。spawn の中の start で 1 回だけ渡す（名簿に載るのはその後）。nullptr 不可
	void setShape( const ICollisionBody* shape );

	// 当たりながら滑って動く（自分の長方形は除いて調べる）
	const CharacterMover& mover() const;
	// 移動の名簿から外し、他のキャラの移動を止めなくなる（何回呼んでもよい。もう戻さない）
	void stopBlocking();
	bool isBlocking() const;

	// Component（動くときに壁・他のキャラを聞く名簿を、共有の棚から引いて滑る移動へ渡す）
	void awake() override;
	// ICollisionBody（shape に聞く）
	BoxCollider collider() const override;
	// IWorldMember（移動の名簿にだけ載る）
	void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) override;
	void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) override;

private:
	const ICollisionBody* mShape;
	CharacterMover mMover;
	CollisionWorld* mBlockingIn; // 載っている名簿（載っていなければ nullptr）
};

#endif
