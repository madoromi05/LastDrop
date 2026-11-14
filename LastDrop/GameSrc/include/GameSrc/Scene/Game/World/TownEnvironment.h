#ifndef INCLUDED_TOWN_ENVIRONMENT_H
#define INCLUDED_TOWN_ENVIRONMENT_H

#include "GameSrc/Map/TownMap.h"
#include "GameSrc/Map/TownGround.h"
#include "GameSrc/Map/Building.h"
#include "GameSrc/Scene/Game/World/WorldBuilder.h"
#include <vector>

class MeshCache;
class TownCache;
class PropFactory;
class NavMesh;
class TownVisibility;
class CollisionWorld;
class HitWorld;
class GameObjectWorld;

// 町（地図・道路と歩道・建物・ゾンビの歩ける床・建物の隠れの表）を作って持つ係
// （SummaryPlan/個別仕様/設計と分割/HighPrioritySplit_Plan.txt 3-1）
// 出撃の始めのロードで一度だけ作り、その後は読まれるだけ（増えも減りもしない）
// 当たり判定の名簿 2 つと物の並びは持たない（町だけの物ではないので GameWorld が持ち、作るときに借りる）
// 置物（車・漁れる場所）は物の並びへ置くので、ここには残らない（持ち主は GameObjectWorld）
class TownEnvironment{
public:
	// 建物・地面の形の読み込みを頼むだけ（非同期）。どれも所有権は受け取らない。nullptr 不可。この係より長く生きていること
	// meshes: 形を借りる倉庫。town: 町の決まった形（歩ける床・隠れの表）を起動中 1 回だけ作って持つ所。props: 置物を組み立てる
	TownEnvironment( MeshCache* meshes, TownCache* town, const PropFactory* props );
	TownEnvironment( const TownEnvironment& ) = delete;
	TownEnvironment& operator=( const TownEnvironment& ) = delete;

	// ── ロード（GameWorld の同じ名前の段から、この順で呼ぶ）──
	// 町（地面・建物・置物・漁れる場所・脱出の車・箱の中のガソリン）を組み立てる（WorldBuilder）
	// escapeCarSpot: 今日の脱出の車の置き場所（TownMap::escapeCarSpots の番号）。lampsLit: 街灯のガラスが光る形で置くか（停電の日は false）
	// 組み立てた後、町の地面を objects の共有の棚へ置く（物の部品が、落ちる先の地面として引く）。1 つの世界につき 1 回だけ呼ぶ
	void build( CollisionWorld* collisionWorld, HitWorld* hitWorld, GameObjectWorld* objects, int escapeCarSpot, bool lampsLit );
	// ゾンビの歩ける床を TownCache から借りる（その車の置き場所の床がまだ無ければ、町の動かない形から作る）。build の後
	// 借りた床を返す（ゾンビの部品が共有の棚から引くので、棚の持ち主が置く）
	NavMesh* buildNavMesh( const CollisionWorld& collisionWorld );
	void buildVisibility( const HitWorld& hitWorld );	// 建物の隠れの表を TownCache から借りる（同じ）。build の後

	// ── 読み取り口 ──
	const TownMap& map() const;							// 町のどこに何を置くか
	const TownGround& ground() const;					// 道路と歩道（足元の材質を聞ける）
	const std::vector< Building >& buildings() const;	// 建てた建物（build の後は増えない）
	const NavMesh& navMesh() const;						// 歩ける床（buildNavMesh の後）
	const TownVisibility& visibility() const;			// 建物・ゾンビの隠れの表（buildVisibility の後）
	const TownVisibility* visibilityIfEnabled() const;	// 隠れの表を使ってよいなら、その表。F6 の PVS を切っている・まだ無いなら nullptr（描く側・アニメの間引きが同じ決まりで読む）

private:
	TownMap mMap;
	TownCache* mTown;					// 借りているだけ
	WorldBuilder mBuilder;				// 町を組み立てる（mMap を借りるので、その後に宣言する）
	TownGround mGround;					// 道路と歩道（見た目・弾の当たり判定・足元の材質）
	std::vector< Building > mBuildings;	// 名簿が要素を指すので、並べ終えた後は増やさない（増やすと並べ直しで要素の場所が変わる）
	NavMesh* mNavMesh;					// buildNavMesh で mTown から借りる（それまでは nullptr）
	const TownVisibility* mVisibility;	// buildVisibility で mTown から借りる（それまでは nullptr）
	int mEscapeCarSpot;					// 今日の脱出の車の置き場所（build で受け取る。歩ける床を選ぶのに使う）
};

#endif
