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
class NavMesh;

// WorldBuilder::build が町を置く先。どれも GameWorld が持ち、build の間だけ借りる（覚えない）。どれも nullptr 不可。
struct WorldBuildTarget{
	TownGround* ground;               // 道路と歩道（置いて、名簿へ載せる）
	std::vector< Building >* buildings; // 建てた建物を後ろへ足す（名簿が要素を指すので、build の後は増やさない）
	CollisionWorld* collisionWorld;   // 移動の当たり判定の名簿
	HitWorld* hitWorld;               // 命中の当たり判定の名簿
	GameObjectWorld* objects;         // 置物を spawn する先
	NavMesh* navMesh;                 // 置き終えた名簿から、歩ける床の形を作る
};

// 町を最初に一度だけ組み立てる係（「どこに何を置くか」。SummaryPlan/個別仕様/GameWorldSplit_Plan.txt 2-2。2026-11-02 GameWorld から分けた）。
// 置き場所は TownMap、置物が何で出来ているかは PropFactory が決める。ここは町の決まり
// （ガソリンを何個置くか）と、置く順番（名簿へ載せ終えてから床の形を作る）を持つ。
// 置いた物の持ち主にはならない（建物の並びは GameWorld、置物は GameObjectWorld が持つ）。
class WorldBuilder{
public:
	// 建物の形の読み込みを頼むだけ（非同期）。どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	// meshes: 形を借りる倉庫。map: 町のどこに何を置くか。props: 置物を組み立てる。
	WorldBuilder( MeshCache* meshes, const TownMap* map, const PropFactory* props );
	WorldBuilder( const WorldBuilder& ) = delete;
	WorldBuilder& operator=( const WorldBuilder& ) = delete;

	// 全データの構築後に一度だけ、地面・建物・車・ガソリンを置き、最後に歩ける床の形を作る。
	// 構築に失敗したデータ（理由は MeshCache がデバッグ出力済み）の物は置かない（その物が無いまま、他の確認はできる）。
	void build( const WorldBuildTarget& target ) const;

private:
	// data の建物を placements の場所へ建てる。data: nullptr 不可。
	void placeBuildings( const WorldBuildTarget& target, const StaticMeshLods& shapes, const std::vector< TownPlacement >& placements ) const;
	// 置物の車と、候補点から選んだ場所のガソリンを置く。
	void placeCarAndFuel( GameObjectWorld* objects ) const;
	// 町で拾える銃を置く（今はライフル 1 丁。始めの銃〈拳銃〉はカバンに入っているので置かない）。
	void placeWeapons( GameObjectWorld* objects ) const;

	const TownMap* mMap;                          // 借りているだけ
	const PropFactory* mProps;                    // 借りているだけ
	StaticMeshLods mBuildingShapes;          // どれも MeshCache から借りる（建物は 1 段だけ）
	StaticMeshLods mPerimeterBuildingShapes; // 町を囲む、足元の板の無い建物
};

#endif
