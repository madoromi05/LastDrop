#ifndef INCLUDED_IWORLD_MEMBER_H
#define INCLUDED_IWORLD_MEMBER_H

class CollisionWorld;
class HitWorld;

// 当たり判定の名簿（移動の CollisionWorld・命中の HitWorld）に自分を載せられるもの。
// どの名簿に・何として載るかは物の側だけが知っている（ゾンビは両方、拾う物は命中だけ、など）。
// 名簿へ載せる入口と外す入口を1か所に揃え、載せ忘れ・外し忘れ（消えた物を名簿が指し続ける）を防ぐ。
// 呼ぶのは持ち主（GameObjectWorld。部品がこれを実装していれば spawn で載せ、flush で外す）。
class IWorldMember{
public:
	virtual ~IWorldMember(){}
	// 自分を名簿へ載せる。どちらも nullptr 不可。所有権は受け取らない。
	virtual void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) = 0;
	// addTo で載せたものを全部外す。消す前に必ず呼ぶ。
	virtual void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) = 0;
};

#endif
