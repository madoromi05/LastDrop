#ifndef INCLUDED_TRIANGLE_MESH_H
#define INCLUDED_TRIANGLE_MESH_H

#include "GameLib/Math/Vector3.h"
#include <vector>

// 弾の当たり判定に使う、三角形の形だけ（位置とつながり）。見た目（UV・法線・色）は持たない。
// GPU へ送った頂点は CPU から読めないので、メッシュを作るときにここへ位置だけ写しておく。
// 全体を囲む箱（AABB）で先に外れを捨て、当たりそうなときだけ三角形を全部調べる。
// 三角形が多くて重くなったら、この中だけを木構造（BVH）に替えればよい（外の呼び方は変わらない）。
class TriangleMesh{
public:
	TriangleMesh();
	// 頂点の位置と三角形のつながりを写す。前の中身は捨てる。indices は3つで三角形1つ。
	void set( const std::vector< GameLib::Math::Vector3 >& positions, const std::vector< unsigned short >& indices );
	bool isEmpty() const;
	// 全部の三角形の隅を、3つずつ corners の後ろへ足す（このメッシュの空間）。形に沿った移動の当たり判定を作るため（MeshWallBuilder）。
	// corners: 出力先。nullptr 不可。
	void appendTriangles( std::vector< GameLib::Math::Vector3 >* corners ) const;

	// origin から delta だけ進む線分が一番早く当たる三角形の時刻（0〜1）と法線（線分の来た側を向く、長さ1）を書く。
	// 座標はこのメッシュの空間（置く前）。当たらなければ false。time / normal: 出力先。nullptr 不可。
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;

private:
	std::vector< GameLib::Math::Vector3 > mPositions;
	std::vector< unsigned short > mIndices;
	GameLib::Math::Vector3 mBoundsMin;
	GameLib::Math::Vector3 mBoundsMax;
};

#endif
