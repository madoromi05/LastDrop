#ifndef INCLUDED_WALL_MESH_H
#define INCLUDED_WALL_MESH_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Collision/WallPiece.h"
#include <vector>

// 建物1軒ぶんの、移動をふさぐ面のかけら（WallPiece）の集まり。メッシュの形に沿った移動の当たり判定
// 全部を囲む範囲も持つ（離れた建物は、かけらを1つも調べずに捨てられる）
// 作るのは MeshWallBuilder。動かない（置いた後に形も場所も変わらない）
class WallMesh{
public:
	WallMesh();
	// かけらを足す。全体を囲む範囲も広げる
	void add( const WallPiece& piece );
	const std::vector< WallPiece >& pieces() const;
	// 全部のかけらを囲む範囲（ワールド座標）。かけらが1つも無いうちは min > max のまま
	const GameLib::Math::Vector3& boundsMin() const;
	const GameLib::Math::Vector3& boundsMax() const;

private:
	std::vector< WallPiece > mPieces;
	GameLib::Math::Vector3 mBoundsMin;
	GameLib::Math::Vector3 mBoundsMax;
};

#endif
