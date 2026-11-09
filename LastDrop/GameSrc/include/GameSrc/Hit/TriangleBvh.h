#ifndef INCLUDED_TRIANGLE_BVH_H
#define INCLUDED_TRIANGLE_BVH_H

#include "GameLib/Math/Vector3.h"
#include <vector>

// 三角形の集まりを、箱の木（BVH）に分けたもの。線分が触れる箱だけを下りて、調べる三角形を数十個に絞る。
// TriangleMesh が持ち、作るのは set の時に 1 回だけ（同じメッシュの建物は MeshCache で 1 つを共有する）。
// 三角形の頂点そのものは持たない（TriangleMesh の位置とつながりを、問い合わせのたびに借りる）。
// 作り方（SummaryPlan/個別仕様/最適化と計測/OptimizationB_Plan.txt 6 章）：三角形の中心を囲む箱の一番長い辺の向きで、
// 中心の並びの真ん中（中央値）で 2 つに分ける。分け方が一言で言え、木の深さがいつも log2(三角形の数) 程度に収まるため。
class TriangleBvh{
public:
	// 木を作る。前の木は捨てる。indices は 3 つで三角形 1 つ（TriangleMesh::set と同じ並び）。
	void build( const std::vector< GameLib::Math::Vector3 >& positions, const std::vector< unsigned short >& indices );
	bool isEmpty() const;
	// 全部の三角形を囲む箱（根の箱）。空なら呼ばないこと。
	const GameLib::Math::Vector3& boundsMin() const;
	const GameLib::Math::Vector3& boundsMax() const;

	// origin から delta だけ進む線分が一番早く当たる三角形の時刻（0〜1）と法線を書く。意味は TriangleMesh::raycast と同じ。
	// positions / indices：build に渡した物と同じ中身であること（借りるだけ）。time / normal: 出力先。nullptr 不可。
	bool raycast( const std::vector< GameLib::Math::Vector3 >& positions, const std::vector< unsigned short >& indices,
		const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;

private:
	// 木の節 1 つ。子の 2 つは配列の隣どうしに置く（左が firstChildOrTriangle、右がその次）。
	struct Node{
		GameLib::Math::Vector3 boundsMin;
		GameLib::Math::Vector3 boundsMax;
		int firstChildOrTriangle; // 葉なら mTriangles の始めの位置、葉でなければ左の子の番号
		int triangleCount;        // 葉なら三角形の数（1 以上）。0 なら葉ではない
	};

	// nodeIndex の節に、mTriangles の first から count 個の三角形を受け持たせ、多ければ 2 つに分けて子を作る。
	// centers：三角形ごとの中心（三角形の番号の順）。
	void split( int nodeIndex, int first, int count, const std::vector< GameLib::Math::Vector3 >& positions,
		const std::vector< unsigned short >& indices, const std::vector< GameLib::Math::Vector3 >& centers );

	std::vector< Node > mNodes;   // [0] が根。空なら木は無い
	std::vector< int > mTriangles; // 三角形の番号を、葉ごとにまとまるよう並べ替えた物
};

#endif
