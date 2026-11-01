#ifndef INCLUDED_ZOMBIE_ATTACK_COMPONENT_H
#define INCLUDED_ZOMBIE_ATTACK_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/ZombieAttack.h"

// ゾンビの腕で引っかく攻撃の段階（ZombieAttack。SummaryPlan/個別仕様/ZombieAttack_Plan.txt）。
// いつ攻撃を始めるか・間合い（ZombieAttackReach）は ZombieBrainComponent が決める。相手に何が起きるかは知らない
// （「このティックに腕が当たったか」を答えるだけで、相手へ伝えるのは GameWorld）。
class ZombieAttackComponent : public Component{
public:
	ZombieAttack& attack(){ return mAttack; }
	const ZombieAttack& attack() const { return mAttack; }

private:
	ZombieAttack mAttack;
};

#endif
