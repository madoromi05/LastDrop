#include "TownGround.h"
#include "MeshCache.h"
#include "StaticMeshData.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Vector3.h"

using namespace GameLib::Math;

namespace{
	// 部品のファイル。並びは TownPiece と同じ。
	// 道路キットの部品は tools/configs/road_*.json で変換（原点は真上から見た中心・一番下）。earth.txt は 1m 四方の無地の板（手書き）。
	const char* const kPieceFiles[ TOWN_PIECE_NUM ] = {
		"Resource/road_straight.txt",
		"Resource/road_center.txt",
		"Resource/road_crosswalk.txt",
		"Resource/road_curb.txt",
		"Resource/road_curb_junction.txt",
		"Resource/road_curb_stub.txt",
		"Resource/road_corner.txt",
		"Resource/road_corner_fill.txt",
		"Resource/road_pavement.txt",
		"Resource/earth.txt",
	};
}

TownGround::TownGround( MeshCache* cache ) {
	ASSERT( cache && "TownGround : cache is null." );
	for ( int i = 0; i < TOWN_PIECE_NUM; ++i ) {
		mPieceData[ i ] = cache->staticMesh( kPieceFiles[ i ] );
	}
}

void TownGround::place( const TownMap& map ) {
	for ( const TownTile& tile : map.groundTiles() ) {
		const StaticMeshData* data = mPieceData[ tile.piece ];
		if ( !data->isReady() ) {
			continue;
		}
		StaticMeshInstance instance( data );
		instance.setPosition( Vector3( tile.placement.x, tile.placement.y, tile.placement.z ) );
		instance.setAngleY( tile.placement.yawDegree );
		instance.setScale( tile.scale );
		mTiles.push_back( instance );
	}
}

void TownGround::draw() const {
	for ( const StaticMeshInstance& tile : mTiles ) {
		tile.draw();
	}
}
