#ifndef INCLUDED_ZOMBIE_ATTACK_REACH_H
#define INCLUDED_ZOMBIE_ATTACK_REACH_H

#include "GameLib/Math/Vector3.h"

// ゾンビの攻撃の間合い（攻撃を始めてよい・腕が届く・どれだけ寄るか）の、距離と向きの判断。状態は持たない（FlatMath と同じ形）
// 「攻撃のアニメがあるか・相手を攻撃してよい状態か・追っているか」など、今の状態を見る判断は Zombie が先に済ませる
// ここは体の位置・向きと相手の位置だけから答える（攻撃してこない・遠くから当たる、の修正の入口。SummaryPlan/個別仕様/設計と分割/ZombieSplit3_Plan.txt の 5 章）
class ZombieAttackReach{
public:
	// 攻撃する体の今の形（Zombie が作って渡す）
	struct Origin{
		GameLib::Math::Vector3 center;    // 体の中心（Zombie::bodyCenter）。向きの差・寄る距離はここから測る
		GameLib::Math::Vector3 reachFrom; // 手の届く距離を測る所（立っていれば center、体を引きずっていれば頭）
		float angleY;                     // 向き（度。0 は +Z）
		bool crawling;                    // 体を引きずっているか（始める距離・届く距離が短い）
	};

	static bool canStart( const Origin& origin, const GameLib::Math::Vector3& target );	// 追いついた相手へ攻撃を始めてよい間合いか（近い・前に居る）
	static bool canHit( const Origin& origin, const GameLib::Math::Vector3& target );	// 腕を振り下ろしたときに、届く所に相手が居るか（近い・前に居る）
	// 手を伸ばしている間に、このティックに寄る長さ（m）。十分近くて寄らなくてよければ 0
	// center: 体の中心（踏み込む前の足元＝根の動きのアンカー）
	static float approachStep( const GameLib::Math::Vector3& center, const GameLib::Math::Vector3& target );
};

#endif
