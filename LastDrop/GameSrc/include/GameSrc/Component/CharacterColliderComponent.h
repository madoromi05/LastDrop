#ifndef INCLUDED_CHARACTER_COLLIDER_COMPONENT_H
#define INCLUDED_CHARACTER_COLLIDER_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Collision/CharacterMover.h"

class CollisionWorld;

// キャラの移動の当たり判定（体を真上から囲む長方形）と、壁・他のキャラに沿って滑る移動（CharacterMover。Collide-and-Slide）。
// 自キャラ・ゾンビが持つ。移動の名簿（CollisionWorld）へ自分で載り、他のキャラの移動を止める（IWorldMember）。
// 長方形の形はキャラごとに違う（自キャラは目と銃の先まで伸ばす、ゾンビは体を引きずる間は胸に置く）ので、
// 形を知っている部品（PlayerComponent・ZombieBrainComponent）へその都度聞く。名簿に載るのはこの部品の方。
// 形を知っている部品は、自分の start で setShape する（その部品は隣の部品を全部使うので最後に付ける。こちらは先に付けて、
// 滑る移動を借りる部品（ZombieChaseComponent など）の start に間に合わせる）。
// 倒れたゾンビの上を歩けるように、途中で名簿から外せる（stopBlocking。外しても、自分が滑って動くのには使える）。
class CharacterColliderComponent : public Component, public ICollisionBody, public IWorldMember{
public:
	// world: 動くときに壁・他のキャラを聞く名簿。所有権は受け取らない。nullptr 不可。この部品より長く生きていること。
	explicit CharacterColliderComponent( const CollisionWorld* world );

	// 長方形の形を答える部品（同じ物の部品）。spawn の中の start で 1 回だけ渡す（名簿に載るのはその後）。nullptr 不可。
	void setShape( const ICollisionBody* shape );

	// 当たりながら滑って動く（自分の長方形は除いて調べる）。
	const CharacterMover& mover() const;
	// 移動の名簿から外し、他のキャラの移動を止めなくなる（何回呼んでもよい。もう戻さない）。
	void stopBlocking();
	bool isBlocking() const;

	// ICollisionBody（shape に聞く）
	virtual BoxCollider collider() const;
	// IWorldMember（移動の名簿にだけ載る）
	virtual void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld );
	virtual void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld );

private:
	const ICollisionBody* mShape;
	CharacterMover mMover;
	CollisionWorld* mBlockingIn; // 載っている名簿（載っていなければ nullptr）
};

#endif
