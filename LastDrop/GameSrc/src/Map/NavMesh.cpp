#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Collision/MoveHeight.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

#include "ThirdParty/recastnavigation/Recast/Recast.h"
#include "ThirdParty/recastnavigation/Detour/DetourNavMesh.h"
#include "ThirdParty/recastnavigation/Detour/DetourNavMeshBuilder.h"
#include "ThirdParty/recastnavigation/Detour/DetourNavMeshQuery.h"
#include "ThirdParty/recastnavigation/Detour/DetourCommon.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>

using namespace GameLib::Math;

namespace{
	// ── 床の形を作る細かさ（Recast の rcConfig。値の理由は ZombieNavMesh_Plan.txt）──
	// 床を区切るますの大きさ（m）。体の半径（0.4m）を 2 ますで表せる細かさ。町（約 130m 四方）で 650 ます四方になる。
	const float kCellSize = 0.2f;
	// 高さのますの大きさ（m）。越えられる段差（0.3m）をちょうど 3 ますで表せる。
	const float kCellHeight = 0.1f;
	// 体の高さ・越えられる段差は、移動の当たり判定と同じ値にする（床の形と実際の歩き方が食い違わないように）。
	const float kAgentHeight = MoveHeight::kBodyTop;
	const float kAgentClimb = MoveHeight::kStepOver;
	// 床の縁を壁から離す長さ（m）。ゾンビの体を真上から囲む長方形（幅の半分 約 0.3m）が斜めを向いても壁にこすらない長さ。
	const float kAgentRadius = 0.4f;
	const float kMaxSlopeDegree = 45.f; // 床は平らなので効かないが、Recast が求める値
	// 小さすぎる床の切れはし（ます数）は捨てる。建物の柱の間などの、歩いて入れない隙間を残さない。
	const int kMinRegionSize = 8;
	// これより小さい床の切れはしは隣とまとめる（多角形の数を減らし、道探しを軽くする）。
	const int kMergeRegionSize = 20;
	// 床の縁の線をまっすぐにするときの、元の形からのずれの上限（ます数）と、1 本の辺の長さの上限（m）。
	const float kEdgeMaxError = 1.3f;
	const float kEdgeMaxLength = 12.f;
	// 多角形 1 つの角の数（Detour の上限 DT_VERTS_PER_POLYGON と同じ）。
	const int kVertsPerPoly = 6;
	// 床の高さを細かく測る間隔（ます数）と、そのずれの上限（高さのます数）。床は平らなので、粗くてよい。
	const float kDetailSampleCells = 6.f;
	const float kDetailSampleMaxErrorCells = 1.f;
	// 床の形を作る範囲の上下（m）。床（0）の下と、体の高さより上に余裕を持たせる。
	const float kBoundsBelowGround = 0.5f;
	const float kBoundsAboveBody = 0.5f;
	const float kGroundHeight = 0.f;
	// 歩ける多角形に付ける印（Detour の道探しは、印が合う多角形だけを通る。印の無い多角形は通らない）。
	const unsigned short kWalkableFlag = 1;

	// ── 道探し（Detour の dtNavMeshQuery）──
	// 一度の道探しで開ける多角形の数の上限（町の端から端までで数百）。
	const int kMaxSearchNodes = 2048;
	// 通り道の多角形の数・曲がる角の数の上限。町の端から端まで足りる数。
	const int kMaxPathPolys = 256;
	const int kMaxCorners = 64;
	// 点の近くの床を探す範囲（半分の大きさ。m）。上下は歩道の段差（0.15m）・自キャラの目の高さで呼ばれても床に届く広さ。
	const float kSearchExtents[ 3 ] = { 2.f, 2.f, 2.f };

	// 箱の角を回る順（ローカルの X・Z の向きの符号）。隣どうしが箱の辺になる。
	const int kBoxCornerNumber = 4;
	const float kCornerSignX[ kBoxCornerNumber ] = { -1.f, 1.f, 1.f, -1.f };
	const float kCornerSignZ[ kBoxCornerNumber ] = { -1.f, -1.f, 1.f, 1.f };
	const int kTriangleCorners = 3;
	const int kCoordinates = 3;
	const double kMillisecondsPerSecond = 1000.0;

	// Recast へ渡す三角形の集まり（頂点の座標を並べた配列と、三角形ごとの頂点の番号と床の種類）。
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

	Vector3 toVector( const float* v ) {
		return Vector3( v[ 0 ], v[ 1 ], v[ 2 ] );
	}

	// 壁のかけら（体の高さで切った形）を、立った板の三角形として足す。床の種類は「歩けない」。
	// かけらは真上から見ると線なので、Recast はその線が通るますに、かけらの高さの柱を立てる（下の床は頭がつかえて歩けなくなる）。
	void addWallPiece( const WallPiece& piece, TriangleSoup* soup ) {
		int first = soup->addVertex( piece.corner[ 0 ] );
		int previous = soup->addVertex( piece.corner[ 1 ] );
		for ( int i = 2; i < piece.cornerNumber; ++i ) {
			int current = soup->addVertex( piece.corner[ i ] );
			soup->addTriangle( first, previous, current, RC_NULL_AREA );
			previous = current;
		}
	}

	// 動かない箱（車）を、床から体の高さまでの柱の 4 つの側面として足す。床の種類は「歩けない」。
	void addBox( const BoxCollider& box, TriangleSoup* soup ) {
		// 箱のローカルの X 軸・Z 軸がワールドで向く向き（SweepMath の boxAxes と同じ。Matrix34::rotateY と同じ回転の向き）。
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

	// 壁の囲む範囲（真上から見た四角）を広げる。
	void growBounds( const Vector3& min, const Vector3& max, Vector3* boundsMin, Vector3* boundsMax ) {
		boundsMin->x = std::min( boundsMin->x, min.x );
		boundsMin->z = std::min( boundsMin->z, min.z );
		boundsMax->x = std::max( boundsMax->x, max.x );
		boundsMax->z = std::max( boundsMax->z, max.z );
	}

	// 壁を囲む範囲いっぱいに、高さ 0 の歩ける床を 2 枚の三角形で敷く。
	void addGround( const Vector3& boundsMin, const Vector3& boundsMax, TriangleSoup* soup ) {
		int a = soup->addVertex( Vector3( boundsMin.x, kGroundHeight, boundsMin.z ) );
		int b = soup->addVertex( Vector3( boundsMax.x, kGroundHeight, boundsMin.z ) );
		int c = soup->addVertex( Vector3( boundsMax.x, kGroundHeight, boundsMax.z ) );
		int d = soup->addVertex( Vector3( boundsMin.x, kGroundHeight, boundsMax.z ) );
		soup->addTriangle( a, b, c, RC_WALKABLE_AREA );
		soup->addTriangle( a, c, d, RC_WALKABLE_AREA );
	}

	// Recast の途中の物を、関数を抜けるときに Recast の関数で消す。
	struct HeightfieldDeleter{ void operator()( rcHeightfield* p ) const { rcFreeHeightField( p ); } };
	struct CompactDeleter{ void operator()( rcCompactHeightfield* p ) const { rcFreeCompactHeightfield( p ); } };
	struct ContourDeleter{ void operator()( rcContourSet* p ) const { rcFreeContourSet( p ); } };
	struct PolyMeshDeleter{ void operator()( rcPolyMesh* p ) const { rcFreePolyMesh( p ); } };
	struct DetailDeleter{ void operator()( rcPolyMeshDetail* p ) const { rcFreePolyMeshDetail( p ); } };

	void logFailure( const char* step ) {
		GameLib::cout << "NavMesh: failed to " << step << GameLib::endl;
	}
}

void NavMesh::NavMeshDeleter::operator()( dtNavMesh* navMesh ) const {
	dtFreeNavMesh( navMesh );
}

void NavMesh::QueryDeleter::operator()( dtNavMeshQuery* query ) const {
	dtFreeNavMeshQuery( query );
}

NavMesh::NavMesh() {
}

NavMesh::~NavMesh() {
}

bool NavMesh::build( const CollisionWorld& world ) {
	mQuery.reset();
	mNavMesh.reset();
	mOutline.clear();
	auto startTime = std::chrono::steady_clock::now();

	// 材料の三角形を集める。床は壁を囲む範囲いっぱいに敷く（町を囲む建物の内側が町）。
	TriangleSoup soup;
	Vector3 boundsMin( FLT_MAX, kGroundHeight - kBoundsBelowGround, FLT_MAX );
	Vector3 boundsMax( -FLT_MAX, kAgentHeight + kBoundsAboveBody, -FLT_MAX );
	for ( const WallMesh& walls : world.wallMeshes() ) {
		if ( walls.pieces().empty() ) {
			continue;
		}
		growBounds( walls.boundsMin(), walls.boundsMax(), &boundsMin, &boundsMax );
		for ( const WallPiece& piece : walls.pieces() ) {
			addWallPiece( piece, &soup );
		}
	}
	if ( boundsMin.x > boundsMax.x ) {
		logFailure( "find any wall (buildings are not placed)" );
		return false;
	}
	for ( const BoxCollider& box : world.boxes() ) {
		addBox( box, &soup );
	}
	addGround( boundsMin, boundsMax, &soup );

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

	// Recast の手順（公式の Sample_SoloMesh と同じ順）：ますに三角形を描く → 歩けない所を除く → 体の半径だけ縁を削る →
	// 床を切れはしに分ける → 縁の線を作る → 多角形にする → 高さを細かく測る。
	rcContext context( false ); // ログ・計時は使わない（かかった時間はこのクラスで測る）
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
	std::unique_ptr< rcPolyMesh, PolyMeshDeleter > polyMesh( rcAllocPolyMesh() );
	if ( !polyMesh || !rcBuildPolyMesh( &context, *contours, config.maxVertsPerPoly, *polyMesh ) ) {
		logFailure( "build the poly mesh" );
		return false;
	}
	std::unique_ptr< rcPolyMeshDetail, DetailDeleter > detail( rcAllocPolyMeshDetail() );
	if ( !detail || !rcBuildPolyMeshDetail( &context, *polyMesh, *compact, config.detailSampleDist, config.detailSampleMaxError, *detail ) ) {
		logFailure( "build the detail mesh" );
		return false;
	}
	for ( int i = 0; i < polyMesh->npolys; ++i ) {
		polyMesh->flags[ i ] = ( polyMesh->areas[ i ] == RC_WALKABLE_AREA ) ? kWalkableFlag : 0;
	}

	// Detour が道探しに使う形へ詰め直す（町全体を 1 枚のタイルにする。町の形は変わらないので分けない）。
	dtNavMeshCreateParams params{};
	params.verts = polyMesh->verts;
	params.vertCount = polyMesh->nverts;
	params.polys = polyMesh->polys;
	params.polyAreas = polyMesh->areas;
	params.polyFlags = polyMesh->flags;
	params.polyCount = polyMesh->npolys;
	params.nvp = polyMesh->nvp;
	params.detailMeshes = detail->meshes;
	params.detailVerts = detail->verts;
	params.detailVertsCount = detail->nverts;
	params.detailTris = detail->tris;
	params.detailTriCount = detail->ntris;
	params.walkableHeight = kAgentHeight;
	params.walkableRadius = kAgentRadius;
	params.walkableClimb = kAgentClimb;
	rcVcopy( params.bmin, polyMesh->bmin );
	rcVcopy( params.bmax, polyMesh->bmax );
	params.cs = config.cs;
	params.ch = config.ch;
	params.buildBvTree = true; // 点の近くの多角形を木で探す（毎回全部を見ない）
	unsigned char* navData = nullptr;
	int navDataSize = 0;
	if ( !dtCreateNavMeshData( &params, &navData, &navDataSize ) ) {
		logFailure( "create the detour data" );
		return false;
	}
	std::unique_ptr< dtNavMesh, NavMeshDeleter > navMesh( dtAllocNavMesh() );
	if ( !navMesh ) {
		dtFree( navData );
		logFailure( "allocate the detour mesh" );
		return false;
	}
	// DT_TILE_FREE_DATA: navData は navMesh が持ち、navMesh を消すときに一緒に消える。
	if ( dtStatusFailed( navMesh->init( navData, navDataSize, DT_TILE_FREE_DATA ) ) ) {
		dtFree( navData );
		logFailure( "init the detour mesh" );
		return false;
	}
	std::unique_ptr< dtNavMeshQuery, QueryDeleter > query( dtAllocNavMeshQuery() );
	if ( !query || dtStatusFailed( query->init( navMesh.get(), kMaxSearchNodes ) ) ) {
		logFailure( "init the query" );
		return false;
	}
	mNavMesh = std::move( navMesh );
	mQuery = std::move( query );
	buildOutline();

	double milliseconds = std::chrono::duration< double >( std::chrono::steady_clock::now() - startTime ).count() * kMillisecondsPerSecond;
	GameLib::cout << "NavMesh: built " << polyMesh->npolys << " polygons in " << milliseconds << " ms" << GameLib::endl;
	return true;
}

bool NavMesh::isReady() const {
	return static_cast< bool >( mNavMesh );
}

bool NavMesh::isWalkable( const Vector3& from, const Vector3& to ) const {
	PROFILE_COUNT( "Path.walkable", 1 ); // 道探しの予算に入れるかを決める数（OptimizationC_Plan.txt Q3）
	if ( !isReady() ) {
		return true;
	}
	dtQueryFilter filter;
	float point[ kCoordinates ];
	toArray( from, point );
	float start[ kCoordinates ];
	dtPolyRef startRef = 0;
	if ( dtStatusFailed( mQuery->findNearestPoly( point, kSearchExtents, &filter, &startRef, start ) ) || startRef == 0 ) {
		return false;
	}
	float end[ kCoordinates ];
	toArray( to, end );
	end[ 1 ] = start[ 1 ]; // 床の上を水平に調べる（目の高さの点を渡されても、床に沿って測る）
	dtRaycastHit hit{};
	if ( dtStatusFailed( mQuery->raycast( startRef, start, end, &filter, 0, &hit ) ) ) {
		return false;
	}
	return hit.t == FLT_MAX; // 床の縁に当たらずに to まで届いた
}

bool NavMesh::findPath( const Vector3& start, const Vector3& goal, std::vector< Vector3 >* out ) const {
	ASSERT( out && "NavMesh : out is null." );
	// 1 回の重さ（Path.ms ÷ Path.find）で、探す処理を途中で区切る作り（OptimizationC_Plan.txt Q2 の B）が要るかを決める。
	PROFILE_TOTAL( "Path.ms" );
	PROFILE_COUNT( "Path.find", 1 );
	out->clear();
	if ( !isReady() ) {
		return false;
	}
	dtQueryFilter filter;
	float point[ kCoordinates ];
	float startPoint[ kCoordinates ];
	float goalPoint[ kCoordinates ];
	dtPolyRef startRef = 0;
	dtPolyRef goalRef = 0;
	toArray( start, point );
	if ( dtStatusFailed( mQuery->findNearestPoly( point, kSearchExtents, &filter, &startRef, startPoint ) ) || startRef == 0 ) {
		return false;
	}
	toArray( goal, point );
	if ( dtStatusFailed( mQuery->findNearestPoly( point, kSearchExtents, &filter, &goalRef, goalPoint ) ) || goalRef == 0 ) {
		return false;
	}
	dtPolyRef polys[ kMaxPathPolys ];
	int polyNumber = 0;
	mQuery->findPath( startRef, goalRef, startPoint, goalPoint, &filter, polys, &polyNumber, kMaxPathPolys );
	if ( polyNumber == 0 ) {
		return false;
	}
	// 着けない（つながっていない床の上が行き先）ときは、たどり着ける最後の多角形の、行き先に一番近い所まで。
	float endPoint[ kCoordinates ];
	dtVcopy( endPoint, goalPoint );
	if ( polys[ polyNumber - 1 ] != goalRef ) {
		mQuery->closestPointOnPoly( polys[ polyNumber - 1 ], goalPoint, endPoint, nullptr );
	}
	float corners[ kMaxCorners * kCoordinates ];
	int cornerNumber = 0;
	if ( dtStatusFailed( mQuery->findStraightPath( startPoint, endPoint, polys, polyNumber, corners, nullptr, nullptr,
		&cornerNumber, kMaxCorners ) ) || cornerNumber == 0 ) {
		return false;
	}
	// 最初の角は出発点そのもの・最後の角は行き先の床の上の点なので、どちらも入れない。最後は本当の行き先にする
	// （ZombiePath が、動いていく行き先に付いていく点として書き換える）。
	for ( int i = 1; i < cornerNumber - 1; ++i ) {
		out->push_back( toVector( &corners[ i * kCoordinates ] ) );
	}
	if ( polys[ polyNumber - 1 ] != goalRef ) {
		out->push_back( toVector( endPoint ) );
	}
	out->push_back( goal );
	return true;
}

const std::vector< Vector3 >& NavMesh::outline() const {
	return mOutline;
}

void NavMesh::buildOutline() {
	const dtNavMesh& navMesh = *mNavMesh;
	for ( int t = 0; t < navMesh.getMaxTiles(); ++t ) {
		const dtMeshTile* tile = navMesh.getTile( t );
		if ( !tile || !tile->header ) {
			continue;
		}
		for ( int p = 0; p < tile->header->polyCount; ++p ) {
			const dtPoly& poly = tile->polys[ p ];
			for ( int e = 0; e < poly.vertCount; ++e ) {
				// 隣の多角形と共有する辺は、番号の小さい方からだけ描く（同じ線を 2 本描かない）。neis は 1 始まりで、0 は隣が無い（床の縁）。
				int neighbor = poly.neis[ e ];
				bool sharedEdge = ( neighbor != 0 ) && !( neighbor & DT_EXT_LINK );
				if ( sharedEdge && neighbor - 1 < p ) {
					continue;
				}
				const float* a = &tile->verts[ poly.verts[ e ] * kCoordinates ];
				const float* b = &tile->verts[ poly.verts[ ( e + 1 ) % poly.vertCount ] * kCoordinates ];
				mOutline.push_back( toVector( a ) );
				mOutline.push_back( toVector( b ) );
			}
		}
	}
}
