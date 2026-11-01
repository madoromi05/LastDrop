#ifndef INCLUDED_ZOMBIE_ATTACK_COMPONENT_H
#define INCLUDED_ZOMBIE_ATTACK_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/ZombieAttack.h"
#include "GameSrc/Zombie/ZombieAttackReach.h"

class SkinnedMeshInstance;
class ZombieStateComponent;
class ZombieLocomotion;
class ZombieCrawler;
class ZombieRootMotionSpan;
class ZombieAnimationSet;
struct ChaseTarget;

// ゾンビの腕で引っかく攻撃（段階は ZombieAttack、間合いは ZombieAttackReach。SummaryPlan/個別仕様/ZombieAttack_Plan.txt）。
// 攻撃を始めるか決めて始め、腕を振る間は相手の方へ寄り、段階を進める。立っている体も、体を引きずっている体も同じ部品で攻撃する。
// いつ「始めるか」を聞くか（考えた後・歩く前）は呼ぶ側（ZombieBrainComponent・ZombieCrawlComponent）が決める。
// 相手に何が起きるかは知らない（「このティックに腕が当たったか」を答えるだけで、相手へ伝えるのは CombatResolver）。
class ZombieAttackComponent : public Component{
public:
	ZombieAttackComponent();

	ZombieAttack& attack(){ return mAttack; }
	const ZombieAttack& attack() const { return mAttack; }

	// 見えている相手が間合いに居て始められるなら、攻撃を始めて true（通り道は捨てる）。始めなければ何もせず false。
	// target: 追う先（nullptr なら始めない）。
	bool tryStart( const ChaseTarget* target );
	// 立って腕を振っている間、相手の方を向き、振り下ろす段階なら寄る（踏み込みは根の動きで別に足される）。
	void approach( const ChaseTarget* target );
	// 攻撃の段階を進める（生きている間、毎ティック）。立っている体は終わったら立つアニメへ戻す。
	void update( const ChaseTarget* target );
	// 倒れ始めた・よろけたなどで、腕を振っていたらやめる。
	void cancel();

	bool isActive() const;          // 攻撃の段階の途中か（構え〜戻しまで）
	// 腕を振っている間か（生きている間だけ）。持ち主は、この間ほかのゾンビに攻撃を始めさせない（TickContext::attacker）。
	bool isAttacking() const;
	bool hitTargetThisTick() const; // 直前の update で相手に腕が当たったか（ダメージの量は ZombieAttack::hitDamage）

	// Component（Transform・SkinnedModel・ZombieState・ZombieChase・ZombieDismember・ZombieRootMotion が要る）
	virtual void start();

private:
	bool canStart( const ChaseTarget* target ) const;
	bool isTargetInReach( const ChaseTarget* target ) const;
	ZombieAttackReach::Origin origin() const;

	ZombieAttack mAttack;
	// ── 同じ物の部品（start で借りる）──
	GameLib::Math::Vector3* mPosition; // 足元の位置（TransformComponent）
	float* mAngleY;                    // 向き（度。TransformComponent）
	SkinnedMeshInstance* mMesh;
	const ZombieStateComponent* mState;
	ZombieLocomotion* mLocomotion;
	const ZombieCrawler* mCrawler;
	ZombieRootMotionSpan* mRootSpan;
	ZombieAnimationSet* mAnimations;
};

#endif
