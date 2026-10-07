#ifndef INCLUDED_BOX_COLLIDER_H
#define INCLUDED_BOX_COLLIDER_H

#include "GameLib/Math/Vector3.h"

// 移動の当たり判定の形。真上から見た、Y軸まわりに回した長方形（XZ 平面）。
// 建物（動かない。1軒で複数持てる）にも、キャラ（体の向きに合わせて回る。BodyFootprint が作る）にも使う。
// 高さは持たない（地面は Y=0 で、ジャンプが無いので上を越えることが無い）。
struct BoxCollider{
	GameLib::Math::Vector3 center; // 中心（ワールド座標。y は判定に使わない）
	float halfWidthX;              // 箱のローカル X 方向の幅の半分（m）
	float halfWidthZ;              // 箱のローカル Z 方向の幅の半分（m）
	float angleY;                  // 向き（度）。StaticMeshInstance::setAngleY と同じ約束
	// 地面（Y=0）からの高さ（m）。移動の判定には使わない（上の説明のとおり 2D で解く）。
	// 当たり判定の表示（CollisionDebugDrawer）が、柱をこの高さで描いて見た目と比べられるようにするためだけに持つ。
	float height;
};

#endif
