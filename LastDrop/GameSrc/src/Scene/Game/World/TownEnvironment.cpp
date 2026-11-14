#include "GameSrc/Scene/Game/World/TownEnvironment.h"
#include "GameSrc/Map/TownCache.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

TownEnvironment::TownEnvironment( MeshCache* meshes, TownCache* town, const PropFactory* props ) :
	mTown( town ),
	mBuilder( meshes, &mMap, props ),
	mGround( meshes ),
	mNavMesh( nullptr ),
	mVisibility( nullptr ),
	mEscapeCarSpot( 0 ) {
	ASSERT( meshes && town && props && "TownEnvironment : an argument is null." );
}

void TownEnvironment::build( CollisionWorld* collisionWorld, HitWorld* hitWorld, GameObjectWorld* objects, int escapeCarSpot, bool lampsLit ) {
	LOAD_SCOPE( "town", "WorldBuilder::build" );
	mEscapeCarSpot = escapeCarSpot;
	WorldBuildTarget target;
	target.ground = &mGround;
	target.buildings = &mBuildings;
	target.collisionWorld = collisionWorld;
	target.hitWorld = hitWorld;
	target.objects = objects;
	mBuilder.build( target, escapeCarSpot, lampsLit );
	objects->provide( &mGround ); // 千切れた部位の部品が、落ちる先の地面として引く（地面だけ。建物・車の上には乗せない）
}

NavMesh* TownEnvironment::buildNavMesh( const CollisionWorld& collisionWorld ) {
	// 建物・車・ゴミ箱・木箱を名簿へ載せ終えた形を材料にする。2 回目からの出撃は作らずに借りる
	mNavMesh = mTown->navMesh( mEscapeCarSpot, collisionWorld );
	return mNavMesh;
}

void TownEnvironment::buildVisibility( const HitWorld& hitWorld ) {
	LOAD_SCOPE( "town", "TownVisibility" );
	mVisibility = mTown->visibility( mBuildings, hitWorld );
}

const TownMap& TownEnvironment::map() const {
	return mMap;
}

const TownGround& TownEnvironment::ground() const {
	return mGround;
}

const std::vector< Building >& TownEnvironment::buildings() const {
	return mBuildings;
}

const NavMesh& TownEnvironment::navMesh() const {
	ASSERT( mNavMesh && "TownEnvironment::navMesh : call buildNavMesh first." );
	return *mNavMesh;
}

const TownVisibility& TownEnvironment::visibility() const {
	ASSERT( mVisibility && "TownEnvironment::visibility : call buildVisibility first." );
	return *mVisibility;
}

const TownVisibility* TownEnvironment::visibilityIfEnabled() const {
	return ( pvsEnabled() && mVisibility && mVisibility->isReady() ) ? mVisibility : nullptr;
}
