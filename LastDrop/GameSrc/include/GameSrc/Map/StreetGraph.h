#ifndef INCLUDED_STREET_GRAPH_H
#define INCLUDED_STREET_GRAPH_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Collision/BoxCollider.h"
#include <vector>

class TownMap;
class HitWorld;

// 町の道の図（交差点を点、隣どうしをつなぐ道を線にしたもの）と、その上の道探し（A*）。
// 建物を回り込む通り道を答えるだけで、誰が歩くかは知らない（ゾンビは ZombiePath が使う）。
// 図に乗る・降りるのは「まっすぐ歩いて通れる（isWalkable）」点だけ。
// 道の上に置いた動かない物（脱出用の車・何度でも拾えるガソリンの置き場）は、少し広げた四隅を点として足す（addObstacle）。
// 交差点を結ぶ道がその物を通り抜けていても、A* が角を回る道を出す（SummaryPlan/個別仕様/ZombieCrowd_Plan.txt の 4 章）。
// ガソリン缶のような小さな物・後から置く物は図に入れない（歩く側が壁に沿って滑り、詰まったら横へよける）。
// 値の理由は SummaryPlan/個別仕様/ZombiePath_Plan.txt。
class StreetGraph{
public:
	// map の交差点とつながりから図を作る（町の形は変わらないので、作るのは1回だけ）。
	explicit StreetGraph( const TownMap& map );

	// 動かない物 box を margin（m）だけ広げた四隅を図の点にし、隣り合う角どうし・ほかの点（交差点・先に足した物の角）と、
	// まっすぐ通れるときだけ結ぶ。box を命中の名簿へ載せた後に呼ぶ（線がその物に当たるかで結ぶかを決めるため）。
	// 町を置くときに 1 回ずつ呼ぶ（線を数十本引くので、毎ティックは呼ばない）。
	void addObstacle( const BoxCollider& box, float margin, const HitWorld& hitWorld );
	// addObstacle で足した角の点（ワールド。確認用の表示に使う）。
	const std::vector< GameLib::Math::Vector3 >& obstacleCorners() const;

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
	// 点 a と b を、両向きの線で結ぶ。
	void link( int a, int b );

	std::vector< Node > mNodes;                         // 交差点のあとに、足した物の角が続く
	std::vector< GameLib::Math::Vector3 > mObstacleCorners;
};

#endif
