#include "GameSrc/Map/TownCache.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

TownCache::TownCache() {
}

// NavMesh・TownVisibility の定義が見える所で破棄する。
TownCache::~TownCache() {
}

NavMesh* TownCache::navMesh( int escapeCarSpot, const CollisionWorld& world ) {
	std::unique_ptr< NavMesh >& navMesh = mNavMeshes[ escapeCarSpot ];
	if ( !navMesh ) {
		LOAD_SCOPE( "town", "NavMesh::build" );
		navMesh = std::make_unique< NavMesh >();
		navMesh->build( world ); // 失敗しても持つ（毎回作り直して、毎回失敗しないように）
	}
	return navMesh.get();
}

const TownVisibility* TownCache::visibility( const std::vector< Building >& buildings, const HitWorld& hitWorld ) {
	if ( !mVisibility ) {
		mVisibility = std::make_unique< TownVisibility >();
		mVisibility->build( buildings );
#ifdef _DEBUG
		LOAD_SCOPE( "town", "TownVisibility::verify" );
		mVisibility->verify( buildings, hitWorld );
#else
		( void )hitWorld; // 確かめるのは Debug だけ
#endif
	}
	return mVisibility.get();
}
