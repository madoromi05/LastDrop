#ifndef INCLUDED_STATIC_MESH_MERGER_H
#define INCLUDED_STATIC_MESH_MERGER_H

#include "GameLib/Math/Matrix34.h"
#include <vector>

class StaticMeshData;

// 同じ動かないメッシュを何枚も置いた物を、置いた後の形のまま1つのメッシュへまとめる（焼き込む）。状態は持たない。
// 道路の板のように「小さい部品を何百枚も並べる」物は、1枚ずつ描くと、頂点は少ないのに描画の回数だけが増える。
// まとめれば、部品の種類ごとに1回（頂点数の上限を超えたら数回）で描ける。
// まとめた後は1枚ずつ動かせない・消せないので、置いた後に変わらない物だけに使う。
// 当たり判定には使わない（まとめる前の1枚ずつが、置き場所と種類を持っているため。TownGround）。
class StaticMeshMerger{
public:
	// source を worlds の置き場所それぞれに置いた形を、merged へ作る（頂点をワールド座標へ移して1つにつなぐ）。
	// 見た目（テクスチャ・材質値）はパーツごとに source のものを写す。描くときは、置き場所を原点・向き 0・等倍にする。
	// source: まとめる元。isReady() 済みで、CPU 側の頂点を残してあること（MeshCache::staticMeshForMerging で借りた物）。
	// worlds: 置き場所の行列（StaticMeshInstance::worldMatrix）。移動・回転・等方の拡大だけであること（法線を回すだけで済ませるため）。
	// merged: 出力先。nullptr 不可。まだ何も作っていない空のデータであること。所有権は受け取らない。
	// 作れたら true。worlds が空なら何も作らず false（merged は isReady() == false のまま）。
	static bool merge( const StaticMeshData& source, const std::vector< GameLib::Math::Matrix34 >& worlds, StaticMeshData* merged );
};

#endif
