#ifndef INCLUDED_STREET_GRAPH_H
#define INCLUDED_STREET_GRAPH_H

#include "GameLib/Math/Vector3.h"
#include <vector>

class TownMap;
class HitWorld;

// 町の道の図（交差点を点、隣どうしをつなぐ道を線にしたもの）と、その上の道探し（A*）。
// 建物を回り込む通り道を答えるだけで、誰が歩くかは知らない（ゾンビは ZombiePath が使う）。
// 図に乗る・降りるのは「まっすぐ歩いて通れる（isWalkable）」交差点だけ。車など道の上の小さな物は図に入れない
// （歩く側が壁に沿って滑ってかわす）。値の理由は SummaryPlan/個別仕様/ZombiePath_Plan.txt。
class StreetGraph{
public:
	// map の交差点とつながりから図を作る（町の形は変わらないので、作るのは1回だけ）。
	explicit StreetGraph( const TownMap& map );

	// from から to まで、まっすぐ歩いて通れるか（腰の高さの線が、建物・車など弾を止める物に当たらないか。キャラは数えない）。
	bool isWalkable( const GameLib::Math::Vector3& from, const GameLib::Math::Vector3& to, const HitWorld& hitWorld ) const;
	// start から goal までの通り道を out に書く（通る交差点の真ん中を順に並べ、最後に goal）。前の中身は捨てる。
	// start・goal のどちらかから通れる交差点が無い（建物のへこみの奥など）か、つながっていなければ false（out は空）。
	// out: nullptr 不可。
	bool findPath( const GameLib::Math::Vector3& start, const GameLib::Math::Vector3& goal, const HitWorld& hitWorld,
		std::vector< GameLib::Math::Vector3 >* out ) const;

private:
	struct Link{
		int to;
		float length; // 交差点どうしの距離（m。水平だけ）
	};
	struct Node{
		GameLib::Math::Vector3 position;
		std::vector< Link > links;
	};
	// 図に乗る・降りる交差点と、そこまでの距離。
	struct Portal{
		int node;
		float distance;
	};

	// point に近い交差点から順に決まった数だけ調べ、まっすぐ通れるものを out に書く（前の中身は捨てる）。
	void findPortals( const GameLib::Math::Vector3& point, const HitWorld& hitWorld, std::vector< Portal >* out ) const;

	std::vector< Node > mNodes;
};

#endif
