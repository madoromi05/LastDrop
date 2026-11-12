#include "GameSrc/Map/NavMeshBuilder.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Collision/MoveHeight.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

#include "ThirdParty/recastnavigation/Recast/Recast.h"
#include "ThirdParty/recastnavigation/Detour/DetourNavMesh.h"
#include "ThirdParty/recastnavigation/Detour/DetourNavMeshBuilder.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <vector>

using namespace GameLib::Math;

namespace{
	// ── 床の形を作る細かさ（Recast の rcConfig） ──
	// 床を区切るますの大きさ（m）。体の半径（0.4m）を 2 ますで表せる細かさ。町（約 130m 四方）で 650 ます四方になる
	const float kCellSize = 0.2f;
	const float kCellHeight = 0.1f;			// 高さのますの大きさ（m）。越えられる段差（0.3m）をちょうど 3 ますで表せる
	// 体の高さ・越えられる段差は、移動の当たり判定と同じ値にする（床の形と実際の歩き方が食い違わないように）
	const float kAgentHeight = MoveHeight::kBodyTop;
	const float kAgentClimb = MoveHeight::kStepOver;
	const float kAgentRadius = 0.4f;		// 床の縁を壁から離す長さ（m）。ゾンビの体を真上から囲む長方形（幅の半分 約 0.3m）が斜めを向いても壁にこすらない長さ
	const float kMaxSlopeDegree = 45.f;		// 床は平らなので効かないが、Recast が求める値
	const int kMinRegionSize = 8;			// 小さすぎる床の切れはし（ます数）は捨てる。建物の柱の間などの、歩いて入れない隙間を残さない
	const int kMergeRegionSize = 20;		// これより小さい床の切れはしは隣とまとめる（多角形の数を減らし、道探しを軽くする）
	// 床の縁の線をまっすぐにするときの、元の形からのずれの上限（ます数）と、1 本の辺の長さの上限（m）
	const float kEdgeMaxError = 1.3f;
	const float kEdgeMaxLength = 12.f;
	const int kVertsPerPoly = 6;			// 多角形 1 つの角の数（Detour の上限 DT_VERTS_PER_POLYGON と同じ）
	// 床の高さを細かく測る間隔（ます数）と、そのずれの上限（高さのます数）。床は平らなので、粗くてよい
	const float kDetailSampleCells = 6.f;
	const float kDetailSampleMaxErrorCells = 1.f;
	// 床の形を作る範囲の上下（m）。床（0）の下と、体の高さより上に余裕を持たせる
	const float kBoundsBelowGround = 0.5f;
	const float kBoundsAboveBody = 0.5f;
	const float kGroundHeight = 0.f;
	const unsigned short kWalkableFlag = 1;	// 歩ける多角形に付ける印（Detour の道探しは、印が合う多角形だけを通る。印の無い多角形は通らない）
	// 箱の角を回る順（ローカルの X・Z の向きの符号）。隣どうしが箱の辺になる
	const int kBoxCornerNumber = 4;
	const float kCornerSignX[ kBoxCornerNumber ] = { -1.f, 1.f, 1.f, -1.f };
	const float kCornerSignZ[ kBoxCornerNumber ] = { -1.f, -1.f, 1.f, 1.f };
	const int kCoordinates = 3;

	// Recast へ渡す三角形の集まり（頂点の座標を並べた配列と、三角形ごとの頂点の番号と床の種類）
	struct TriangleSoup{
		std::vector< float > vertices;
		std::vector< int > triangles;
		std::vector< unsigned char > areas;

		int addVertex( const Vector3& v ) {
			vertices.push_back( v.x );
			vertices.push_back( v.y );
			vertices.push_back( v.z );
			return static_cast< int >( vertices.size() ) / kCoordinates - 1;
		}
		void addTriangle( int a, int b, int c, unsigned char area ) {
			triangles.push_back( a );
			triangles.push_back( b );
			triangles.push_back( c );
			areas.push_back( area );
		}
		int vertexNumber() const {
			return static_cast< int >( vertices.size() ) / kCoordinates;
		}
		int triangleNumber() const {
			return static_cast< int >( areas.size() );
		}
	};

	void toArray( const Vector3& v, float* out ) {
		out[ 0 ] = v.x;
		out[ 1 ] = v.y;
		out[ 2 ] = v.z;
	}

	// 壁のかけら（体の高さで切った形）を、立った板の三角形として足す。床の種類は「歩けない」
	// かけらは真上から見ると線なので、Recast はその線が通るますに、かけらの高さの柱を立てる（下の床は頭がつかえて歩けなくなる）
	void addWallPiece( const WallPiece& piece, TriangleSoup* soup ) {
		int first = soup->addVertex( piece.corner[ 0 ] );
		int previous = soup->addVertex( piece.corner[ 1 ] );
		for ( int i = 2; i < piece.cornerNumber; ++i ) {
			int current = soup->addVertex( piece.corner[ i ] );
			soup->addTriangle( first, previous, current, RC_NULL_AREA );
			previous = current;
		}
	}

	// 動かない箱（車）を、床から体の高さまでの柱の 4 つの側面として足す。床の種類は「歩けない」
	void addBox( const BoxCollider& box, TriangleSoup* soup ) {
		// 箱のローカルの X 軸・Z 軸がワールドで向く向き（SweepMath の boxAxes と同じ。Matrix34::rotateY と同じ回転の向き）
		float c = GameLib::Math::cos( box.angleY );
		float s = GameLib::Math::sin( box.angleY );
		Vector3 axisX( c, 0.f, -s );
		Vector3 axisZ( s, 0.f, c );
		int bottom[ kBoxCornerNumber ];
		int top[ kBoxCornerNumber ];
		for ( int i = 0; i < kBoxCornerNumber; ++i ) {
			Vector3 corner( box.center.x, kGroundHeight, box.center.z );
			corner.madd( axisX, kCornerSignX[ i ] * box.halfWidthX );
			corner.madd( axisZ, kCornerSignZ[ i ] * box.halfWidthZ );
			bottom[ i ] = soup->addVertex( corner );
			corner.y = kAgentHeight;
			top[ i ] = soup->addVertex( corner );
		}
		for ( int i = 0; i < kBoxCornerNumber; ++i ) {
			int next = ( i + 1 ) % kBoxCornerNumber;
			soup->addTriangle( bottom[ i ], bottom[ next ], top[ next ], RC_NULL_AREA );
			soup->addTriangle( bottom[ i ], top[ next ], top[ i ], RC_NULL_AREA );
		}
	}

	// 壁の囲む範囲（真上から見た四角）を広げる
	void growBounds( const Vector3& min, const Vector3& max, Vector3* boundsMin, Vector3* boundsMax ) {
		boundsMin->x = std::min( boundsMin->x, min.x );
		boundsMin->z = std::min( boundsMin->z, min.z );
		boundsMax->x = std::max( boundsMax->x, max.x );
		boundsMax->z = std::max( boundsMax->z, max.z );
	}

	// 壁を囲む範囲いっぱいに、高さ 0 の歩ける床を 2 枚の三角形で敷く
	void addGround( const Vector3& boundsMin, const Vector3& boundsMax, TriangleSoup* soup ) {
		int a = soup->addVertex( Vector3( boundsMin.x, kGroundHeight, boundsMin.z ) );
		int b = soup->addVertex( Vector3( boundsMax.x, kGroundHeight, boundsMin.z ) );
		int c = soup->addVertex( Vector3( boundsMax.x, kGroundHeight, boundsMax.z ) );
		int d = soup->addVertex( Vector3( boundsMin.x, kGroundHeight, boundsMax.z ) );
		soup->addTriangle( a, b, c, RC_WALKABLE_AREA );
		soup->addTriangle( a, c, d, RC_WALKABLE_AREA );
	}

	// Recast の途中の物を、関数を抜けるときに Recast の関数で消す
	struct HeightfieldDeleter{ void operator()( rcHeightfield* p ) const { rcFreeHeightField( p ); } };
	struct CompactDeleter{ void operator()( rcCompactHeightfield* p ) const { rcFreeCompactHeightfield( p ); } };
	struct ContourDeleter{ void operator()( rcContourSet* p ) const { rcFreeContourSet( p ); } };
	struct PolyMeshDeleter{ void operator()( rcPolyMesh* p ) const { rcFreePolyMesh( p ); } };
	struct DetailDeleter{ void operator()( rcPolyMeshDetail* p ) const { rcFreePolyMeshDetail( p ); } };

	void logFailure( const char* step ) {
		GameLib::cout << "NavMesh: failed to " << step << GameLib::endl;
	}

	// 材料の三角形を soup へ集め、それを囲む範囲を boundsMin / boundsMax へ書く。床は壁を囲む範囲いっぱいに敷く（町を囲む建物の内側が町）
	// 壁が 1 つも無ければ false（建物を置く前に呼んだ）
	bool gatherTriangles( const CollisionWorld& world, TriangleSoup* soup, Vector3* boundsMin, Vector3* boundsMax ) {
		*boundsMin = Vector3( FLT_MAX, kGroundHeight - kBoundsBelowGround, FLT_MAX );
		*boundsMax = Vector3( -FLT_MAX, kAgentHeight + kBoundsAboveBody, -FLT_MAX );
		for ( const WallMesh& walls : world.wallMeshes() ) {
			if ( walls.pieces().empty() ) {
				continue;
			}
			growBounds( walls.boundsMin(), walls.boundsMax(), boundsMin, boundsMax );
			for ( const WallPiece& piece : walls.pieces() ) {
				addWallPiece( piece, soup );
			}
		}
		if ( boundsMin->x > boundsMax->x ) {
			logFailure( "find any wall (buildings are not placed)" );
			return false;
		}
		for ( const BoxCollider& box : world.boxes() ) {
			addBox( box, soup );
		}
		addGround( *boundsMin, *boundsMax, soup );
		return true;
	}

	// Recast の設定（細かさ・体の大きさ・範囲）。値の理由は上の定数
	rcConfig makeConfig( const Vector3& boundsMin, const Vector3& boundsMax ) {
		rcConfig config{};
		config.cs = kCellSize;
		config.ch = kCellHeight;
		config.walkableSlopeAngle = kMaxSlopeDegree;
		config.walkableHeight = static_cast< int >( std::ceil( kAgentHeight / kCellHeight ) );
		config.walkableClimb = static_cast< int >( std::floor( kAgentClimb / kCellHeight ) );
		config.walkableRadius = static_cast< int >( std::ceil( kAgentRadius / kCellSize ) );
		config.maxEdgeLen = static_cast< int >( kEdgeMaxLength / kCellSize );
		config.maxSimplificationError = kEdgeMaxError;
		config.minRegionArea = kMinRegionSize * kMinRegionSize;
		config.mergeRegionArea = kMergeRegionSize * kMergeRegionSize;
		config.maxVertsPerPoly = kVertsPerPoly;
		config.detailSampleDist = kDetailSampleCells * kCellSize;
		config.detailSampleMaxError = kDetailSampleMaxErrorCells * kCellHeight;
		toArray( boundsMin, config.bmin );
		toArray( boundsMax, config.bmax );
		rcCalcGridSize( config.bmin, config.bmax, config.cs, &config.width, &config.height );
		return config;
	}

	// Recast の手順（公式の Sample_SoloMesh と同じ順）：ますに三角形を描く → 歩けない所を除く → 体の半径だけ縁を削る →
	// 床を切れはしに分ける → 縁の線を作る → 多角形にする → 高さを細かく測る。どこかで失敗したら false（理由はログ）
	bool buildPolygons( const rcConfig& config, const TriangleSoup& soup, rcPolyMesh* polyMesh, rcPolyMeshDetail* detail ) {
		rcContext context( false ); // ログ・計時は使わない（かかった時間は NavMesh が測る）
		std::unique_ptr< rcHeightfield, HeightfieldDeleter > heightfield( rcAllocHeightfield() );
		if ( !heightfield || !rcCreateHeightfield( &context, *heightfield, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch ) ) {
			logFailure( "create the heightfield" );
			return false;
		}
		if ( !rcRasterizeTriangles( &context, soup.vertices.data(), soup.vertexNumber(), soup.triangles.data(), soup.areas.data(),
			soup.triangleNumber(), *heightfield, config.walkableClimb ) ) {
			logFailure( "rasterize triangles" );
			return false;
		}
		rcFilterLowHangingWalkableObstacles( &context, config.walkableClimb, *heightfield );
		rcFilterLedgeSpans( &context, config.walkableHeight, config.walkableClimb, *heightfield );
		rcFilterWalkableLowHeightSpans( &context, config.walkableHeight, *heightfield );

		std::unique_ptr< rcCompactHeightfield, CompactDeleter > compact( rcAllocCompactHeightfield() );
		if ( !compact || !rcBuildCompactHeightfield( &context, config.walkableHeight, config.walkableClimb, *heightfield, *compact ) ) {
			logFailure( "build the compact heightfield" );
			return false;
		}
		heightfield.reset(); // ここから先は使わない（町全体ぶんの大きさがあるので早めに放す）
		if ( !rcErodeWalkableArea( &context, config.walkableRadius, *compact ) ) {
			logFailure( "erode the walkable area" );
			return false;
		}
		if ( !rcBuildDistanceField( &context, *compact ) ||
			!rcBuildRegions( &context, *compact, 0, config.minRegionArea, config.mergeRegionArea ) ) {
			logFailure( "build regions" );
			return false;
		}
		std::unique_ptr< rcContourSet, ContourDeleter > contours( rcAllocContourSet() );
		if ( !contours || !rcBuildContours( &context, *compact, config.maxSimplificationError, config.maxEdgeLen, *contours ) ) {
			logFailure( "build contours" );
			return false;
		}
		if ( !rcBuildPolyMesh( &context, *contours, config.maxVertsPerPoly, *polyMesh ) ) {
			logFailure( "build the poly mesh" );
			return false;
		}
		if ( !rcBuildPolyMeshDetail( &context, *polyMesh, *compact, config.detailSampleDist, config.detailSampleMaxError, *detail ) ) {
			logFailure( "build the detail mesh" );
			return false;
		}
		for ( int i = 0; i < polyMesh->npolys; ++i ) {
			polyMesh->flags[ i ] = ( polyMesh->areas[ i ] == RC_WALKABLE_AREA ) ? kWalkableFlag : 0;
		}
		return true;
	}

	// Detour が道探しに使う形へ詰め直す（町全体を 1 枚のタイルにする。町の形は変わらないので分けない）。失敗したら nullptr
	dtNavMesh* packForDetour( const rcConfig& config, const rcPolyMesh& polyMesh, const rcPolyMeshDetail& detail ) {
		dtNavMeshCreateParams params{};
		params.verts = polyMesh.verts;
		params.vertCount = polyMesh.nverts;
		params.polys = polyMesh.polys;
		params.polyAreas = polyMesh.areas;
		params.polyFlags = polyMesh.flags;
		params.polyCount = polyMesh.npolys;
		params.nvp = polyMesh.nvp;
		params.detailMeshes = detail.meshes;
		params.detailVerts = detail.verts;
		params.detailVertsCount = detail.nverts;
		params.detailTris = detail.tris;
		params.detailTriCount = detail.ntris;
		params.walkableHeight = kAgentHeight;
		params.walkableRadius = kAgentRadius;
		params.walkableClimb = kAgentClimb;
		rcVcopy( params.bmin, polyMesh.bmin );
		rcVcopy( params.bmax, polyMesh.bmax );
		params.cs = config.cs;
		params.ch = config.ch;
		params.buildBvTree = true; // 点の近くの多角形を木で探す（毎回全部を見ない）
		unsigned char* navData = nullptr;
		int navDataSize = 0;
		if ( !dtCreateNavMeshData( &params, &navData, &navDataSize ) ) {
			logFailure( "create the detour data" );
			return nullptr;
		}
		dtNavMesh* navMesh = dtAllocNavMesh();
		if ( !navMesh ) {
			dtFree( navData );
			logFailure( "allocate the detour mesh" );
			return nullptr;
		}
		// DT_TILE_FREE_DATA: navData は navMesh が持ち、navMesh を消すときに一緒に消える
		if ( dtStatusFailed( navMesh->init( navData, navDataSize, DT_TILE_FREE_DATA ) ) ) {
			dtFree( navData );
			dtFreeNavMesh( navMesh );
			logFailure( "init the detour mesh" );
			return nullptr;
		}
		return navMesh;
	}
}

dtNavMesh* NavMeshBuilder::build( const CollisionWorld& world, int* polygonCount ) {
	ASSERT( polygonCount && "NavMeshBuilder::build : polygonCount is null." );
	TriangleSoup soup;
	Vector3 boundsMin;
	Vector3 boundsMax;
	if ( !gatherTriangles( world, &soup, &boundsMin, &boundsMax ) ) {
		return nullptr;
	}
	const rcConfig config = makeConfig( boundsMin, boundsMax );
	std::unique_ptr< rcPolyMesh, PolyMeshDeleter > polyMesh( rcAllocPolyMesh() );
	if ( !polyMesh ) {
		logFailure( "build the poly mesh" );
		return nullptr;
	}
	std::unique_ptr< rcPolyMeshDetail, DetailDeleter > detail( rcAllocPolyMeshDetail() );
	if ( !detail ) {
		logFailure( "build the detail mesh" );
		return nullptr;
	}
	if ( !buildPolygons( config, soup, polyMesh.get(), detail.get() ) ) {
		return nullptr;
	}
	*polygonCount = polyMesh->npolys;
	return packForDetour( config, *polyMesh, *detail );
}
