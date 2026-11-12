#ifndef INCLUDED_TRIANGLE_MESH_H
#define INCLUDED_TRIANGLE_MESH_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Hit/TriangleBvh.h"
#include <vector>

// 弾の当たり判定に使う、三角形の形だけ（位置とつながり）。見た目（UV・法線・色）は持たない
// GPU へ送った頂点は CPU から読めないので、メッシュを作るときにここへ位置だけ写しておく
// 線分の判定は箱の木（TriangleBvh）で、線分が触れる箱の三角形だけを調べる（外の呼び方は同じ）
class TriangleMesh{
public:
	// 頂点の位置と三角形のつながりを写し、箱の木を作る。前の中身は捨てる。indices は3つで三角形1つ
	// 木を作る分だけ重い（三角形の数 n に n log n）ので、読み込みの時に 1 回だけ呼ぶこと
	void set( const std::vector< GameLib::Math::Vector3 >& positions, const std::vector< unsigned short >& indices );
	bool isEmpty() const;
	// 全部の三角形の隅を、3つずつ corners の後ろへ足す（このメッシュの空間）。形に沿った移動の当たり判定を作るため（MeshWallBuilder）
	// corners: 出力先。nullptr 不可
	void appendTriangles( std::vector< GameLib::Math::Vector3 >* corners ) const;

	// origin から delta だけ進む線分が一番早く当たる三角形の時刻（0〜1）と法線（線分の来た側を向く、長さ1）を書く
	// 座標はこのメッシュの空間（置く前）。当たらなければ false。time / normal: 出力先。nullptr 不可
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;

private:
#ifndef NDEBUG
	// 木の答えが、三角形を全部調べた答えと同じかを、乱数の線分で確かめる（Debug だけ。違えば ASSERT）
	void verifyTree() const;
	// 三角形を全部調べる、木を使わない判定（verifyTree の手本）
	bool raycastAll( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;
#endif

	std::vector< GameLib::Math::Vector3 > mPositions;
	std::vector< unsigned short > mIndices;
	TriangleBvh mTree;
};

#endif
