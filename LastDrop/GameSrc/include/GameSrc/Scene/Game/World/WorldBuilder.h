#ifndef INCLUDED_WORLD_BUILDER_H
#define INCLUDED_WORLD_BUILDER_H

#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLods.h"
#include <vector>

class MeshCache;
class StaticMeshData;
class PropFactory;
class TownGround;
class Building;
class CollisionWorld;
class HitWorld;
class GameObjectWorld;

// WorldBuilder::build が町を置く先。どれも GameWorld が持ち、build の間だけ借りる（覚えない）。どれも nullptr 不可。
struct WorldBuildTarget{
	TownGround* ground;               // 道路と歩道（置いて、名簿へ載せる）
	std::vector< Building >* buildings; // 建てた建物を後ろへ足す（名簿が要素を指すので、build の後は増やさない）
	CollisionWorld* collisionWorld;   // 移動の当たり判定の名簿
	HitWorld* hitWorld;               // 命中の当たり判定の名簿
	GameObjectWorld* objects;         // 置物を spawn する先
};

// 町を最初に一度だけ組み立てる係（「どこに何を置くか」。SummaryPlan/個別仕様/設計と分割/GameWorldSplit_Plan.txt 2-2。2026-11-02 GameWorld から分けた）。
// 置き場所は TownMap、置物が何で出来ているかは PropFactory、漁れる場所の中身の出やすさは LootTable が決める。ここは町の決まり
// （中身を決める乱数の種・ガソリンを入れる箱の選び方）を持つ。歩ける床の形は作らない（GameWorld が TownCache から借りる）。
// 置いた物の持ち主にはならない（建物の並びは GameWorld、置物は GameObjectWorld が持つ）。
class WorldBuilder{
public:
	// 建物の形の読み込みを頼むだけ（非同期）。どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	// meshes: 形を借りる倉庫。map: 町のどこに何を置くか。props: 置物を組み立てる。
	WorldBuilder( MeshCache* meshes, const TownMap* map, const PropFactory* props );
	WorldBuilder( const WorldBuilder& ) = delete;
	WorldBuilder& operator=( const WorldBuilder& ) = delete;

	// 全データの構築後に一度だけ、地面・建物・置物の車・漁れる場所（ゴミ箱・木箱）・脱出の車を置く。
	// 漁れる場所のうち RaidParams::fuelCanCount か所に、ガソリンを 1 個ずつ入れる（FuelEscapeLoading_Plan.txt Q15）。
	// escapeCarSpot: 脱出の車を置く所（TownMap::escapeCarSpots の番号）。
	// lampsLit: 街灯をガラスが光る形で置くか（停電の日は消えた形。StreetLamp_Plan.txt Q4）。
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）の物は置かない（その物が無いまま、他の確認はできる）。
	void build( const WorldBuildTarget& target, int escapeCarSpot, bool lampsLit ) const;

private:
	// data の建物を placements の場所へ建てる。data: nullptr 不可。
	void placeBuildings( const WorldBuildTarget& target, const StaticMeshLods& shapes, const std::vector< TownPlacement >& placements ) const;
	// 漁れる場所（ゴミ箱・木箱）を置き、中身を決める。ガソリンを入れる箱を選ぶのもここ。
	// 地面に落ちているガソリンとライフルは置かない。
	void placeLootSpots( GameObjectWorld* objects ) const;
	// 今日の脱出の車を置く（給油できる車。漁れない）。
	void placeEscapeCar( GameObjectWorld* objects, int escapeCarSpot ) const;

	const TownMap* mMap;                          // 借りているだけ
	const PropFactory* mProps;                    // 借りているだけ
	StaticMeshLods mBuildingShapes; // MeshCache から借りる（建物は 1 段だけ・足元の板の無い形。ブロックの中も町を囲むのも同じ）
};

#endif
