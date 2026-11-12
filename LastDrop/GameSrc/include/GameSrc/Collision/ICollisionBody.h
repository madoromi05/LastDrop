#ifndef INCLUDED_ICOLLISION_BODY_H
#define INCLUDED_ICOLLISION_BODY_H

#include "GameSrc/Collision/BoxCollider.h"

// 動いて、他のキャラの移動を止めるもの（主人公・ゾンビ）
// CollisionWorld が掃引のたびに今の形を問い合わせる（位置・向きを写して持たないので、ずれない）
class ICollisionBody{
public:
	virtual ~ICollisionBody(){}
	virtual BoxCollider collider() const = 0;	// 今の移動の当たり判定（体を真上から囲む長方形。体の向きに合わせて回る。ワールド座標）
};

#endif
