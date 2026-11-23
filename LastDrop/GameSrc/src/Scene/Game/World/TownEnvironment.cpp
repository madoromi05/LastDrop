#include "GameSrc/Scene/Game/World/TownEnvironment.h"
#include "GameSrc/Map/TownCache.h"
#include "GameSrc/Map/TownEdits.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/TownVisibility.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

TownEnvironment::TownEnvironment( MeshCache* meshes, TownCache* town, const TownEdits* edits, const PropFactory* props ) :
	mTown( town ),
	mMap( edits, townEditLimits() ),
	mBuilder( meshes, &mMap, props ),
	mGround( meshes ),
	mNavMesh( nullptr ),
	mVisibility( nullptr ),
	mEscapeCarSpot( 0 ),
	mBuildingsEdited( false ) {
	ASSERT( meshes && town && props && "TownEnvironment : an argument is null." );
	for ( const std::string& warning : mMap.editWarnings() ) {
		GameLib::cout << "TownEdit: " << warning.c_str() << GameLib::endl;	// 手直しのうち使わなかった行（出撃は止めない）
	}
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

NavMesh* TownEnvironment::buildNavMesh( const CollisionWorld& collisionWorld, float agentRadius ) {
	// 建物・車・ゴミ箱・木箱を名簿へ載せ終えた形を材料にする。事前に書き出した床が今の町と合えば読み、2 回目からの出撃は作らずに借りる
	mNavMesh = mTown->navMesh( mEscapeCarSpot, mMap, collisionWorld, agentRadius );
	return mNavMesh;
}

void TownEnvironment::buildVisibility( const HitWorld& hitWorld ) {
	LOAD_SCOPE( "town", "TownVisibility" );
	mVisibility = mTown->visibility( mMap, mBuildings, hitWorld );
}

void TownEnvironment::moveBuilding( int index, const GameLib::Math::Vector3& position, float angleY ) {
	ASSERT( index >= 0 && index < static_cast< int >( mBuildings.size() ) && "TownEnvironment::moveBuilding : bad index." );
	mBuildings[ index ].moveTo( position, angleY );
	mBuildingsEdited = true;
}

void TownEnvironment::setBuildingHidden( int index, bool hidden ) {
	ASSERT( index >= 0 && index < static_cast< int >( mBuildings.size() ) && "TownEnvironment::setBuildingHidden : bad index." );
	mBuildings[ index ].setHidden( hidden );
	mBuildingsEdited = true;
}

GameObject* TownEnvironment::spawnBuildingPreview( GameObjectWorld* objects, const TownPlacement& at ) const {
	return mBuilder.spawnBuildingPreview( objects, at );
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
	return ( pvsEnabled() && !mBuildingsEdited && mVisibility && mVisibility->isReady() ) ? mVisibility : nullptr;
}
