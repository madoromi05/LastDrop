#include "TownGround.h"
#include "MeshCache.h"
#include "StaticMeshData.h"
#include "StaticMeshMerger.h"

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

	// 部品の種類ごとの足元の材質（足音の音色）。並びは TownPiece と同じ。車道は砂利、歩道はコンクリート。
	const GroundMaterial kPieceMaterials[] = {
		GROUND_GRAVEL,   // 直線の道路
		GROUND_GRAVEL,   // 交差点の真ん中
		GROUND_GRAVEL,   // 横断歩道
		GROUND_CONCRETE, // 歩道の帯
		GROUND_CONCRETE, // 歩道の帯（交差点の辺）
		GROUND_CONCRETE, // 歩道の帯（交差点の角）
		GROUND_CONCRETE, // 角の丸い歩道
		GROUND_GRAVEL,   // 丸い歩道の下に敷く車道
		GROUND_CONCRETE, // ブロックの内側の床
		GROUND_GRAVEL,   // 町の外の土（歩けない場所。部品のすき間から下の土に届いたときのために、車道に合わせる）
	};
	static_assert( sizeof( kPieceMaterials ) / sizeof( kPieceMaterials[ 0 ] ) == TOWN_PIECE_NUM, "kPieceMaterials must list every TownPiece" );

	// 足元の材質を調べる縦の線分の両端（足元からの高さ、m）。
	// キャラは高さ 0 のまま動くので、歩道の上では面の方が足元より高い。面を上から探せるよう、足元より上から下ろす。
	// 下の端は、車道より少し低く敷いてある町の外の土まで届かせる。
	const float kProbeTop = 1.f;
	const float kProbeBottom = -1.f;
}

TownGround::TownGround( MeshCache* cache ) {
	ASSERT( cache && "TownGround : cache is null." );
	for ( int i = 0; i < TOWN_PIECE_NUM; ++i ) {
		// 置いた後に1つへまとめるので、頂点を CPU に残す形で借りる（どれも数個〜数十個の頂点しか無い小さい部品）。
		mPieceData[ i ] = cache->staticMeshForMerging( kPieceFiles[ i ] );
	}
}

void TownGround::place( const TownMap& map ) {
	std::vector< Matrix34 > worlds[ TOWN_PIECE_NUM ]; // 種類ごとの、置いた部品の置き場所（まとめる材料）
	for ( const TownTile& tile : map.groundTiles() ) {
		const StaticMeshData* data = mPieceData[ tile.piece ];
		if ( !data->isReady() ) {
			continue;
		}
		StaticMeshInstance instance( data, Vector3( tile.placement.x, tile.placement.y, tile.placement.z ), tile.placement.yawDegree );
		instance.setScale( tile.scale );
		mTiles.push_back( { instance, tile.piece } );
		worlds[ tile.piece ].push_back( instance.worldMatrix() );
	}
	// 種類ごとに、置いた全部を1つのメッシュへまとめる（描くのはこちら）。1枚も置けなかった種類は作らない。
	for ( int piece = 0; piece < TOWN_PIECE_NUM; ++piece ) {
		auto merged = std::make_unique< StaticMeshData >();
		if ( !StaticMeshMerger::merge( *mPieceData[ piece ], worlds[ piece ], merged.get() ) ) {
			continue;
		}
		mMergedMeshes.push_back( StaticMeshInstance( merged.get() ) );
		mMergedData.push_back( std::move( merged ) );
	}
}

void TownGround::draw( const ViewFrustum& frustum ) const {
	for ( const StaticMeshInstance& merged : mMergedMeshes ) {
		merged.draw( frustum );
	}
}

bool TownGround::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return nearestTile( origin, delta, time, normal ) != nullptr;
}

bool TownGround::materialBelow( const Vector3& position, GroundMaterial* material ) const {
	Vector3 top( position.x, position.y + kProbeTop, position.z );
	Vector3 down( 0.f, kProbeBottom - kProbeTop, 0.f );
	float time;
	Vector3 normal;
	const Tile* tile = nearestTile( top, down, &time, &normal );
	if ( !tile ) {
		return false;
	}
	*material = kPieceMaterials[ tile->piece ];
	return true;
}

const TownGround::Tile* TownGround::nearestTile( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	const Tile* nearest = nullptr;
	for ( const Tile& tile : mTiles ) {
		float t;
		Vector3 n;
		if ( !tile.mesh.raycast( origin, delta, &t, &n ) ) {
			continue;
		}
		if ( nearest && t >= *time ) {
			continue;
		}
		*time = t;
		*normal = n;
		nearest = &tile;
	}
	return nearest;
}
