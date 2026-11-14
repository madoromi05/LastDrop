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
#include "GameSrc/Weapon/WeaponCatalog.h"
#include <vector>

#include "GameLib/GameLib.h"
#include "Framework.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 建物は tools/configs/building_noground.json の scale で、ドアが人の背丈に合う大きさ（ドアの高さ約 2.0m）に縮めてある
	// ブロックの中も町を囲むのも、足元の歩道の板を除いた同じ形
	// 足元はどちらも TownMap が敷く床（歩道の高さ）が見える。板があると、隣の板や床と重なって面がちらつくため
	const char* kBuildingFile = "Resource/Model/Map/building_noground.txt";
	const GroundMaterial kBuildingMaterial = GROUND_CONCRETE;	// 建物の壁の材質（着弾音）。どちらもレンガの建物なので、コンクリートの音で鳴らす

	// 箱に必ず入れる物。ガソリンは 1 個、ライフルは弾倉いっぱい
	const ItemStack kFuelStack = { ITEM_FUEL, 1 };
	ItemStack loadedRifle() {
		const WeaponSpec& rifle = weaponSpec( WEAPON_RIFLE );
		ItemStack stack = { rifle.item, 1 };
		stack.loadedRounds = rifle.magazineCapacity;
		return stack;
	}

	// count か所を 0〜spotCount−1 から選び、その場所の「必ず入れる物」に stack を足す（offset: 選んだ番号に足す数）
	void addToRandomSpots( std::vector< std::vector< ItemStack > >* guaranteed, int offset, int spotCount, int count,
		const ItemStack& stack, Random* random ) {
		for ( int spot : SpawnPicker::pick( spotCount, count, static_cast< unsigned >( random->getInt() ) ) ) {
			( *guaranteed )[ offset + spot ].push_back( stack );
		}
	}
}

WorldBuilder::WorldBuilder( MeshCache* meshes, const TownMap* map, const PropFactory* props ) :
	mMap( map ),
	mProps( props ),
	// 建物は粗い形（LOD）を持たない（三角形の 85% が窓枠・ガラスの小さな部品で、Decimate で窓が崩れるため）
	mBuildingShapes( meshes->staticMesh( kBuildingFile ) ) {
	ASSERT( meshes && map && props && "WorldBuilder : an argument is null." );
}

void WorldBuilder::build( const WorldBuildTarget& target, int escapeCarSpot, bool lampsLit ) const {
	target.ground->place( *mMap );
	target.hitWorld->addBlocker( target.ground ); // 地面も弾・視線を止める（地面全体で1つの物）
	target.collisionWorld->setGround( target.ground ); // キャラは地面の高さに乗る（歩道の段差）
	placeBuildings( target, mBuildingShapes, mMap->buildings() );
	placeBuildings( target, mBuildingShapes, mMap->perimeterBuildings() );
	// 弾を止める物として登録する。建物を並べ終えてから行う（push_back の途中で登録すると、
	// 並べ直しで要素の場所が変わり、名簿が古い場所を指してしまうため）
	for ( const Building& building : *target.buildings ) {
		target.hitWorld->addBlocker( &building );
	}
	placeLootSpots( target.objects );
	// 消火栓は見た目の置物（置けなくても〈形の構築の失敗〉その場所に無いだけで遊べる）
	for ( const TownPlacement& p : mMap->fireHydrants() ) {
		mProps->spawnFireHydrant( target.objects, p.position(), p.yawDegree );
	}
	// 街灯の柱も置けなくても遊べる（同じ）。照らす光は柱の物の LightComponent が持つので、柱が置けなければ光も出ない
	for ( const TownPlacement& p : mMap->streetLights() ) {
		mProps->spawnStreetLamp( target.objects, p.position(), p.yawDegree, lampsLit );
	}
	placeEscapeCar( target.objects, escapeCarSpot ); // 町に置く車はこの 1 台だけ（乗れない置物の車は置かない）
}

void WorldBuilder::placeBuildings( const WorldBuildTarget& target, const StaticMeshLods& shapes,
const std::vector< TownPlacement >& placements ) const {
	// 当たり判定を三角形から作るので、構築に失敗した建物は置かない
	if ( !shapes.full()->isReady() ) {
		return;
	}
	std::vector< Building >* buildings = target.buildings;
	buildings->reserve( buildings->size() + placements.size() );
	for ( const TownPlacement& p : placements ) {
		buildings->emplace_back( shapes, p.position(), p.yawDegree, kBuildingMaterial );
		buildings->back().addCollidersTo( target.collisionWorld );
	}
}

void WorldBuilder::placeLootSpots( GameObjectWorld* objects ) const {
	// 置き場所は毎日同じで、中身だけ毎日変わる（起動からの時間を種にする。脱出の車の置き場所と同じ）
	// 置けなくても（形の構築の失敗）その場所が無いだけで遊べる
	Random random = Random::create( static_cast< int >( Framework::instance().time() ) );
	// ガソリンを入れる箱を、漁れる場所（ゴミ箱 → 木箱の通し番号）から fuelCanCount か所選ぶ
	// 置物の車には物を入れない（改訂 2）
	// 1 か所に 1 個。選ぶのは場所の番号なので、形の構築に失敗して置けなかった場所に当たると、その分だけ町のガソリンが減る
	// ライフルは木箱の中から rifleCount か所。ガソリンと同じ木箱に当たったら両方入れる（2 × 3 に入る）
	const int dumpsterCount = static_cast< int >( mMap->dumpsters().size() );
	const int crateCount = static_cast< int >( mMap->crates().size() );
	const int spotCount = dumpsterCount + crateCount;
	const RaidParams& params = raidParams();
	ASSERT( params.fuelCanCount >= 0 && params.fuelCanCount <= spotCount && "WorldBuilder : fuelCanCount is more than the loot spots (add spots to TownMap)." );
	ASSERT( params.rifleCount >= 0 && params.rifleCount <= crateCount && "WorldBuilder : rifleCount is more than the crates (add crates to TownMap)." );
	std::vector< std::vector< ItemStack > > guaranteed( spotCount );
	const int kFromFirstSpot = 0;
	addToRandomSpots( &guaranteed, kFromFirstSpot, spotCount, params.fuelCanCount, kFuelStack, &random );
	addToRandomSpots( &guaranteed, dumpsterCount, crateCount, params.rifleCount, loadedRifle(), &random ); // 木箱はゴミ箱の後ろの番号
	int spot = 0;
	for ( const TownPlacement& p : mMap->dumpsters() ) {
		mProps->spawnLootContainer( objects, LootTableId::Dumpster, p.position(), p.yawDegree, &random, guaranteed[ spot++ ] );
	}
	for ( const TownPlacement& p : mMap->crates() ) {
		mProps->spawnLootContainer( objects, LootTableId::Crate, p.position(), p.yawDegree, &random, guaranteed[ spot++ ] );
	}
}

void WorldBuilder::placeEscapeCar( GameObjectWorld* objects, int escapeCarSpot ) const {
	const std::vector< TownEscapeCarSpot >& spots = mMap->escapeCarSpots();
	ASSERT( escapeCarSpot >= 0 && escapeCarSpot < static_cast< int >( spots.size() ) && "WorldBuilder : bad escape car spot." );
	const TownPlacement& p = spots[ escapeCarSpot ].placement;
	mProps->spawnEscapeCar( objects, p.position(), p.yawDegree, raidParams().requiredFuel );
}
