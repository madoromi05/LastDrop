#ifndef INCLUDED_STATIC_MESH_LODS_H
#define INCLUDED_STATIC_MESH_LODS_H

class StaticMeshData;
class MeshCache;

// 1 つの置物の、細かさの段（LOD）ごとの形（SummaryPlan/個別仕様/最適化と計測/OptimizationC_Plan.txt 8 章）。0 が元の細かい形で、番号が大きいほど粗い
// 粗い形は変換ツールが <名前>_lod1.txt・_lod2.txt に書き出す（tools/configs の "lods"）。どの段も同じモデルの空間・同じ画像
// 形は MeshCache から借りるだけの小さな値（コピーしてよい）。読み込みが済んでいない段・無い段を頼まれたら、細かい側の段を返す
// 当たり判定は元の形だけを使う（見た目だけを粗くする）
class StaticMeshLods{
public:
	static const int kLevelNumber = 3; // 元の形＋粗い形 2 つ

	explicit StaticMeshLods( const StaticMeshData* full );						// full だけの 1 段（粗い形を持たない置物）。full: nullptr 不可
	static StaticMeshLods request( MeshCache* meshes, const char* filename );	// filename の形と、粗い形（<名前>_lod1.txt・_lod2.txt）を meshes に頼む。粗い形のファイルは在ること。meshes: nullptr 不可

	const StaticMeshData* full() const;				// 元の細かい形（当たり判定・囲む箱はこれで作る）
	const StaticMeshData* level( int level ) const;	// level 段の形。その段が無い・読み込み前なら、その段より細かい段で使える物

private:
	const StaticMeshData* mLevels[ kLevelNumber ]; // 借りているだけ。無い段は nullptr
};

#endif
