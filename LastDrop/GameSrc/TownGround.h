#ifndef INCLUDED_TOWN_GROUND_H
#define INCLUDED_TOWN_GROUND_H

#include "TownMap.h"
#include "StaticMeshInstance.h"
#include <vector>

class MeshCache;
class StaticMeshData;

// 町の地面（道路・歩道・ブロックの床・町の外の土）の見た目。TownMap の並べ方どおりに部品を置いて描く。
// 当たり判定は持たない（どれも歩いて越えられる高さなので移動を止めず、視線・弾も止めない）。
// 部品の種類とファイルの対応はここだけが知っている（TownMap は「どの種類をどこに」だけを決める）。
class TownGround{
public:
	// 部品の読み込みを頼むだけ（非同期）。cache->update() が true を返してから place を呼ぶ。
	// cache: 部品の設計図を借りる倉庫。所有権は受け取らない。nullptr 不可。この地面より長く生きていること。
	explicit TownGround( MeshCache* cache );

	// map の並べ方どおりに部品を置く。構築に失敗した種類の部品は置かない（理由は MeshCache がデバッグ出力済み）。
	void place( const TownMap& map );
	void draw() const;

private:
	const StaticMeshData* mPieceData[ TOWN_PIECE_NUM ]; // MeshCache から借りる。添え字は TownPiece
	std::vector< StaticMeshInstance > mTiles;
};

#endif
