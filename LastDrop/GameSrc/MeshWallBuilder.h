#ifndef INCLUDED_MESH_WALL_BUILDER_H
#define INCLUDED_MESH_WALL_BUILDER_H

#include "GameLib/Math/Vector3.h"
#include "WallMesh.h"

class StaticMeshData;

// 動かないメッシュの三角形から、形に沿った移動の当たり判定（WallMesh）を作る。状態は持たない。
// 三角形を1枚ずつ見た目と同じ置き場所・向きでワールドへ置き、体の高さの範囲（MoveHeight）で切り取ってかけらにする。
// 範囲の外にある三角形（足元の歩道の板、頭より上の庇）は、かけらにならない＝移動をふさがない。
// 箱1つで囲む MeshBoundsBox と違い、へこんだ入口や斜めの壁もそのままの形で当たる。
class MeshWallBuilder{
public:
	// data: 形。nullptr 不可。isReady() 済みであること（三角形は upload のときに残るため）。
	// position / angleY: StaticMeshInstance に渡すのと同じ置き場所と向き（度）。拡大率は 1 の前提（MeshBoundsBox と同じ）。
	static WallMesh make( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY );
};

#endif
