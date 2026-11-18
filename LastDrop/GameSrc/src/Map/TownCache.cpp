#include "GameSrc/Map/TownCache.h"
#ifdef _DEBUG
#include "GameSrc/DebugMode/TownVisibilityCheck.h"
#endif
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

TownCache::TownCache() {
}

// NavMesh・TownVisibility の定義が見える所で破棄する
TownCache::~TownCache() {
}

NavMesh* TownCache::navMesh( int escapeCarSpot, const CollisionWorld& world, float agentRadius ) {
	std::unique_ptr< NavMesh >& navMesh = mNavMeshes[ escapeCarSpot ];
	if ( !navMesh ) {
		navMesh = std::make_unique< NavMesh >();
	}
	if ( navMesh->agentRadius() != agentRadius ) {
		LOAD_SCOPE( "town", "NavMesh::build" );
		navMesh->build( world, agentRadius ); // 失敗しても持つ（同じ長さで毎回作り直して、毎回失敗しないように）
	}
	return navMesh.get();
}

const TownVisibility* TownCache::visibility( const std::vector< Building >& buildings, const HitWorld& hitWorld ) {
	if ( !mVisibility ) {
		mVisibility = std::make_unique< TownVisibility >();
		mVisibility->build( buildings );
#ifdef _DEBUG
		LOAD_SCOPE( "town", "TownVisibility::verify" );
		TownVisibilityCheck::run( *mVisibility, buildings, hitWorld ); // 3D の線で表を確かめる（DebugMode）
#else
		( void )hitWorld; // 確かめるのは Debug だけ
#endif
	}
	return mVisibility.get();
}
