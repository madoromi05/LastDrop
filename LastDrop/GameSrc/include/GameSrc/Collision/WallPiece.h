#ifndef INCLUDED_WALL_PIECE_H
#define INCLUDED_WALL_PIECE_H

#include "GameLib/Math/Vector3.h"

// 移動をふさぐ面のかけら1つ。メッシュの三角形1枚を、体の高さの範囲（MoveHeight）で切り取った形（ワールド座標）
// 体は「真上から見た長方形を、体の高さぶん立ち上げた柱」で、水平にしか動かない
// だから「柱と三角形が当たるか」は、「長方形と、切り取った形を真上から見た多角形が当たるか」と同じになる（2D で解ける）
// 作るのは MeshWallBuilder。SweepMath が判定に使い、CollisionDebugDrawer が線で描く
struct WallPiece{
	static const int kMaxCorners = 5;	// 三角形（3 隅）を上と下の高さで切ると、切るたびに隅が1つ増える

	GameLib::Math::Vector3 corner[ kMaxCorners ];     // 切り取った形の隅（回りの順。判定は x・z だけを使い、y は表示のために残す）
	int cornerNumber;
	GameLib::Math::Vector3 edgeNormal[ kMaxCorners ]; // 真上から見た辺の法線（水平・長さ1）。同じ向き・逆向きは1本にまとめてある。分離軸に使う
	int edgeNormalNumber;                             // まっすぐ立った壁なら 1 本
	GameLib::Math::Vector3 boundsMin;                 // 隅を囲む範囲（遠いかけらを先に捨てるため）
	GameLib::Math::Vector3 boundsMax;
};

#endif
