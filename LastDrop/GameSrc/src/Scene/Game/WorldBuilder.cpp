#include "GameSrc/Scene/Game/WorldBuilder.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Map/TownGround.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Map/SpawnPicker.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/Interaction/FuelTankComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"
#include "Framework.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 建物は tools/configs/building.json の scale で、ドアが人の背丈に合う大きさ（ドアの高さ約 2.0m）に縮めてある。
	const char* kBuildingFile = "Resource/building.txt";
	// 町を囲む建物。同じ建物から足元の歩道の板を除いた形（tools/configs/building_noground.json）。
	// 板があると、壁どうしを付けて並べたときに隣の板と重なって面がちらつくため。
	const char* kPerimeterBuildingFile = "Resource/building_noground.txt";
	// 建物の壁の材質（着弾音）。どちらもレンガの建物なので、コンクリートの音で鳴らす。
	const GroundMaterial kBuildingMaterial = GROUND_CONCRETE;

	// 車に入れると走れるようになるガソリンの数。候補の場所（TownMap）から、毎回この数だけ選んで置く（SpawnPicker）。
	const int kRequiredFuel = 3;

	Vector3 toPosition( const TownPlacement& placement ) {
		return Vector3( placement.x, placement.y, placement.z );
	}
}

WorldBuilder::WorldBuilder( MeshCache* meshes, const TownMap* map, const PropFactory* props ) :
	mMap( map ),
	mProps( props ),
	// 建物は粗い形（LOD）を持たない（OptimizationC_Plan.txt 13-3。三角形の 85% が窓枠・ガラスの小さな部品で、Decimate で窓が崩れるため）。
	mBuildingShapes( meshes->staticMesh( kBuildingFile ) ),
	mPerimeterBuildingShapes( meshes->staticMesh( kPerimeterBuildingFile ) ) {
	ASSERT( meshes && map && props && "WorldBuilder : an argument is null." );
}

const FuelTankComponent* WorldBuilder::build( const WorldBuildTarget& target ) const {
	target.ground->place( *mMap );
	target.hitWorld->addBlocker( target.ground ); // 地面も弾・視線を止める（地面全体で1つの物）
	target.collisionWorld->setGround( target.ground ); // キャラは地面の高さに乗る（歩道の段差）
	placeBuildings( target, mBuildingShapes, mMap->buildings() );
	placeBuildings( target, mPerimeterBuildingShapes, mMap->perimeterBuildings() );
	// 弾を止める物として登録する。建物を並べ終えてから行う（push_back の途中で登録すると、
	// 並べ直しで要素の場所が変わり、名簿が古い場所を指してしまうため）。
	for ( const Building& building : *target.buildings ) {
		target.hitWorld->addBlocker( &building );
	}
	const FuelTankComponent* fuelTank = placeMissionObjects( target.objects );
	placeWeapons( target.objects );
	// 建物・車・ガソリンの置き場を名簿へ載せ終えてから、それを材料に歩ける床の形を作る（ZombieNavMesh_Plan.txt）。
	target.navMesh->build( *target.collisionWorld );
	return fuelTank;
}

void WorldBuilder::placeBuildings( const WorldBuildTarget& target, const StaticMeshLods& shapes,
const std::vector< TownPlacement >& placements ) const {
	// 当たり判定を三角形から作るので、構築に失敗した建物は置かない。
	if ( !shapes.full()->isReady() ) {
		return;
	}
	std::vector< Building >* buildings = target.buildings;
	buildings->reserve( buildings->size() + placements.size() );
	for ( const TownPlacement& p : placements ) {
		buildings->emplace_back( shapes, toPosition( p ), p.yawDegree, kBuildingMaterial );
		buildings->back().addCollidersTo( target.collisionWorld );
	}
}

void WorldBuilder::placeWeapons( GameObjectWorld* objects ) const {
	const TownPlacement& rifle = mMap->rifle();
	mProps->spawnWeaponDrop( objects, WEAPON_RIFLE, toPosition( rifle ), rifle.yawDegree ); // 置けなくても（形の構築の失敗）拳銃で遊べる
}

const FuelTankComponent* WorldBuilder::placeMissionObjects( GameObjectWorld* objects ) const {
	// 車を置けなかった場合はクリアできないが、他の確認はできる。
	const TownPlacement& car = mMap->escapeCar();
	GameObject* escapeCar = mProps->spawnEscapeCar( objects, toPosition( car ), car.yawDegree, kRequiredFuel );
	const TownPlacement& station = mMap->endlessFuel();
	mProps->spawnFuelStation( objects, toPosition( station ), station.yawDegree );
	// 遊ぶたびに違う場所に出す（起動からの時間を種にする）。
	unsigned seed = Framework::instance().time();
	const std::vector< TownPlacement >& spawnPoints = mMap->fuelSpawnPoints();
	for ( int index : SpawnPicker::pick( static_cast< int >( spawnPoints.size() ), kRequiredFuel, seed ) ) {
		const TownPlacement& p = spawnPoints[ index ];
		mProps->spawnFuel( objects, toPosition( p ), p.yawDegree );
	}
	return escapeCar ? escapeCar->getComponent< FuelTankComponent >() : nullptr;
}
