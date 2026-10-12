#ifndef INCLUDED_MESH_WALL_BUILDER_H
#define INCLUDED_MESH_WALL_BUILDER_H

#include "GameLib/Math/Vector3.h"
#include "WallMesh.h"

class StaticMeshInstance;

// 動かないメッシュの三角形から、形に沿った移動の当たり判定（WallMesh）を作る。状態は持たない。
// 三角形を1枚ずつ見た目と同じ行列（StaticMeshInstance::worldMatrix）でワールドへ置き、体の高さの範囲（MoveHeight）で切り取ってかけらにする。
// 範囲の外にある三角形（足元の歩道の板、頭より上の庇）は、かけらにならない＝移動をふさがない。
// 箱1つで囲む MeshBoundsBox と違い、へこんだ入口や斜めの壁もそのままの形で当たる。
class MeshWallBuilder{
public:
	// mesh: 置いた見た目。その形（三角形）と置き場所の行列から作るので、見た目を回しても拡大しても当たり判定が付いてくる。
	static WallMesh make( const StaticMeshInstance& mesh );
};

#endif
