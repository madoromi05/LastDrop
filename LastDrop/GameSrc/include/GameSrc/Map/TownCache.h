#ifndef INCLUDED_TOWN_CACHE_H
#define INCLUDED_TOWN_CACHE_H

#include <map>
#include <memory>
#include <vector>

class NavMesh;
class TownVisibility;
class CollisionWorld;
class HitWorld;
class Building;

// 町の決まった形（置き場所は TownMap の定数なので毎日同じ）から作る重い物を、起動中に 1 回だけ作って持つ
// （FuelEscapeLoading_Plan.txt 5 章 L2・L3。出撃のたびに作り直すと、Debug で数秒かかっていた）。GameResources が持つ
//   ・ゾンビの歩ける床（NavMesh）：脱出の車の置き場所で床が変わるので、置き場所ごとに 1 つ（候補は 2 か所なので最大 2 つ）
//   ・建物の隠れの表（TownVisibility）：建物だけで決まるので 1 つ。Debug の確かめ（verify）も作った 1 回だけ
// 作るための材料（当たり判定の名簿・建物）は、最初に頼んだ世界のものを使う。2 回目からは材料を見ずに、覚えた物を返す
// TownMap の定数を変えたら、起動し直すまで古い形のまま（今も同じ。起動中に町の形は変えない）
class TownCache{
public:
	TownCache();
	~TownCache();
	TownCache( const TownCache& ) = delete;
	TownCache& operator=( const TownCache& ) = delete;

	// escapeCarSpot（TownMap::escapeCarSpots の番号）に車を置いた町の歩ける床。まだ無ければ world の動かない形から作る
	// 返す床はこの持ち主が生きている間使える（作るのに失敗した床も返す。isReady が false で、ゾンビはまっすぐ寄るだけになる）
	NavMesh* navMesh( int escapeCarSpot, const CollisionWorld& world );
	// 建物の隠れの表。まだ無ければ buildings から作る（Debug では、作った後に hitWorld で一度だけ確かめる）
	const TownVisibility* visibility( const std::vector< Building >& buildings, const HitWorld& hitWorld );

private:
	std::map< int, std::unique_ptr< NavMesh > > mNavMeshes;
	std::unique_ptr< TownVisibility > mVisibility;
};

#endif
