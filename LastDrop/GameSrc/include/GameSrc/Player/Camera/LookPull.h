#ifndef INCLUDED_LOOK_PULL_H
#define INCLUDED_LOOK_PULL_H

#include "GameLib/Math/Vector3.h"

class FirstPersonCamera;

// 視点を決まった時間をかけて、ある所（つかんだゾンビの顔）へ向ける係
// 向けている間もマウスで回せる（回した分はそのまま足される。ここはカメラの今の向きへ、残りを足していくだけ）
class LookPull{
public:
	LookPull();

	void start( const FirstPersonCamera& camera, const GameLib::Math::Vector3& target );	// camera の今の向きから target（ワールド座標）の方へ向け始める。続けて呼ぶと、その時点の向きから向け直す
	void update( FirstPersonCamera* camera );												// 1ティックぶん camera を回す（キャラを動かした後）。向けていなければ何もしない。camera: nullptr 不可

private:
	// 残っている回す量（度）と、残りのティック数（0 なら向けていない）
	float mYaw;
	float mPitch;
	int mTicks;
};

#endif
