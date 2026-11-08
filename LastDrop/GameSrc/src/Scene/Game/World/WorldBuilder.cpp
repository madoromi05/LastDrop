#include "GameSrc/Scene/Game/World/WorldBuilder.h"
#include "GameSrc/Render/MeshCache.h"
#include "GameSrc/Map/TownGround.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Interaction/PropFactory.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Map/SpawnPicker.h"
#include "GameSrc/Raid/RaidParams.h"

#include "GameLib/GameLib.h"
#include "Framework.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 建物は tools/configs/building.json の scale で、ドアが人の背丈に合う大きさ（ドアの高さ約 2.0m）に縮めてある。
	const char* kBuildingFile = "Resource/Model/Map/building.txt";
	// 町を囲む建物。同じ建物から足元の歩道の板を除いた形（tools/configs/building_noground.json）。
	// 板があると、壁どうしを付けて並べたときに隣の板と重なって面がちらつくため。
	const char* kPerimeterBuildingFile = "Resource/Model/Map/building_noground.txt";
	// 建物の壁の材質（着弾音）。どちらもレンガの建物なので、コンクリートの音で鳴らす。
	const GroundMaterial kBuildingMaterial = GROUND_CONCRETE;

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

void WorldBuilder::build( const WorldBuildTarget& target, int escapeCarSpot ) const {
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
	placeLootSpots( target.objects );
	// 消火栓は見た目の置物（置けなくても〈形の構築の失敗〉その場所に無いだけで遊べる）。
	for ( const TownPlacement& p : mMap->fireHydrants() ) {
		mProps->spawnFireHydrant( target.objects, toPosition( p ), p.yawDegree );
	}
	placeEscapeCar( target.objects, escapeCarSpot ); // 町に置く車はこの 1 台だけ（乗れない置物の車は置かない）
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

void WorldBuilder::placeLootSpots( GameObjectWorld* objects ) const {
	// 置き場所は毎日同じで、中身だけ毎日変わる（R3_Plan.txt Q3。起動からの時間を種にする。脱出の車の置き場所と同じ）。
	// 置けなくても（形の構築の失敗）その場所が無いだけで遊べる。
	Random random = Random::create( static_cast< int >( Framework::instance().time() ) );
	// ガソリンを入れる箱を、漁れる場所（ゴミ箱 → 木箱の通し番号）から fuelCanCount か所選ぶ（FuelEscapeLoading_Plan.txt Q15）。
	// 置物の車には物を入れない（改訂 2）。
	// 1 か所に 1 個。選ぶのは場所の番号なので、形の構築に失敗して置けなかった場所に当たると、その分だけ町のガソリンが減る。
	const int spotCount = static_cast< int >( mMap->dumpsters().size() + mMap->crates().size() );
	const int fuelCount = raidParams().fuelCanCount;
	ASSERT( fuelCount >= 0 && fuelCount <= spotCount && "WorldBuilder : fuelCanCount is more than the loot spots (add spots to TownMap)." );
	std::vector< bool > withFuel( spotCount, false );
	for ( int spot : SpawnPicker::pick( spotCount, fuelCount, static_cast< unsigned >( random.getInt() ) ) ) {
		withFuel[ spot ] = true;
	}
	int spot = 0;
	for ( const TownPlacement& p : mMap->dumpsters() ) {
		mProps->spawnLootContainer( objects, LootTableId::Dumpster, toPosition( p ), p.yawDegree, &random, withFuel[ spot++ ] );
	}
	for ( const TownPlacement& p : mMap->crates() ) {
		mProps->spawnLootContainer( objects, LootTableId::Crate, toPosition( p ), p.yawDegree, &random, withFuel[ spot++ ] );
	}
}

void WorldBuilder::placeEscapeCar( GameObjectWorld* objects, int escapeCarSpot ) const {
	const std::vector< TownEscapeCarSpot >& spots = mMap->escapeCarSpots();
	ASSERT( escapeCarSpot >= 0 && escapeCarSpot < static_cast< int >( spots.size() ) && "WorldBuilder : bad escape car spot." );
	const TownPlacement& p = spots[ escapeCarSpot ].placement;
	mProps->spawnEscapeCar( objects, toPosition( p ), p.yawDegree, raidParams().requiredFuel );
}
