#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Map/NavMeshBuilder.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

#include "ThirdParty/recastnavigation/Detour/DetourNavMesh.h"
#include "ThirdParty/recastnavigation/Detour/DetourNavMeshQuery.h"
#include "ThirdParty/recastnavigation/Detour/DetourCommon.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>

using namespace GameLib::Math;

namespace{
	// ── 道探し（Detour の dtNavMeshQuery）──
	// 一度の道探しで開ける多角形の数の上限（町の端から端までで数百）。
	const int kMaxSearchNodes = 2048;
	// 通り道の多角形の数・曲がる角の数の上限。町の端から端まで足りる数。
	const int kMaxPathPolys = 256;
	const int kMaxCorners = 64;
	// 点の近くの床を探す範囲（半分の大きさ。m）。上下は歩道の段差（0.15m）・自キャラの目の高さで呼ばれても床に届く広さ。
	const float kSearchExtents[ 3 ] = { 2.f, 2.f, 2.f };
	const int kCoordinates = 3;
	const double kMillisecondsPerSecond = 1000.0;

	void toArray( const Vector3& v, float* out ) {
		out[ 0 ] = v.x;
		out[ 1 ] = v.y;
		out[ 2 ] = v.z;
	}

	Vector3 toVector( const float* v ) {
		return Vector3( v[ 0 ], v[ 1 ], v[ 2 ] );
	}

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

	// 床の形を作るのは NavMeshBuilder（Recast）。ここは受け取って、道を探す道具（Detour の query）を付ける。
	int polygonCount = 0;
	std::unique_ptr< dtNavMesh, NavMeshDeleter > navMesh( NavMeshBuilder::build( world, &polygonCount ) );
	if ( !navMesh ) {
		return false; // 理由は NavMeshBuilder がログへ出した
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
	GameLib::cout << "NavMesh: built " << polygonCount << " polygons in " << milliseconds << " ms" << GameLib::endl;
	return true;
}

bool NavMesh::isReady() const {
	return static_cast< bool >( mNavMesh );
}

bool NavMesh::isWalkable( const Vector3& from, const Vector3& to ) const {
	PROFILE_COUNT( "Path.walkable", 1 ); // 道探しの予算に入れるかを決める数
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
	// 1 回の重さ（Path.ms ÷ Path.find）で、探す処理を途中で区切る作りが要るかを決める。
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
