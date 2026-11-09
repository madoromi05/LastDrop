#ifndef INCLUDED_ZOMBIE_CROWD_H
#define INCLUDED_ZOMBIE_CROWD_H

#include "GameLib/Math/Vector3.h"
#include <vector>

class GameObject;

// 立って歩けるゾンビの位置の並び（群れ）と、近くの仲間から離れる向き（分離）。仕様は SummaryPlan/個別仕様/ゾンビ/ZombieCrowd_Plan.txt の 3 章。
// GameWorld が毎ティック作り直して、ゾンビ全員の update へ渡す（ゾンビは名簿を知らないので、仲間の位置はここから読む）。
// 分離は向きだけを答える。体を動かすのは今までどおり CharacterMover の掃引（押し出し方式にはしない）。
// 近くの仲間は全員を順に見て探す（上限 30 体なので、全員どうしで 900 回の距離。空間分割は要らない）。
class ZombieCrowd{
public:
	// 並びを空にする（毎ティックの頭に）。
	void clear();
	// 立って歩けるゾンビを 1 体足す。owner: 自分を数えないための目印（借りるだけ）。position: 体の中心の足元（水平だけ使う）。
	void add( const GameObject* owner, const GameLib::Math::Vector3& position );
	// self から見て、radius より近い仲間から離れる向き（水平。長さは 0〜1。近いほど強い）。
	// 仲間それぞれから自分への向きに（1 − 距離 ÷ radius）を掛けて足し、長さが 1 を超えたら 1 にする。
	// self: 自分（並びに入っていれば数えない。nullptr 可）。
	GameLib::Math::Vector3 separation( const GameObject* self, const GameLib::Math::Vector3& position, float radius ) const;

private:
	struct Member{
		const GameObject* owner;
		GameLib::Math::Vector3 position;
	};
	std::vector< Member > mMembers;
};

#endif
