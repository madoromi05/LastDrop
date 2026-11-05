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

// ゾンビの攻撃（段階は ZombieAttack、間合いは ZombieAttackReach。SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 3・9 章）。
// 立っている体は「つかむ → 噛む → 離す」（噛むときは首と背骨を突き出す。ZombieFlinch::lunge）、
// 体を引きずっている体は足元へ腕を伸ばして 1 回当てる。どちらもこの部品が攻撃を始めるか決めて始め、手を伸ばす間は相手の方へ寄り、段階を進める。
// いつ「始めるか」を聞くか（考えた後・歩く前）は呼ぶ側（ZombieBrainComponent・ZombieCrawlComponent）が決める。
// 相手に何が起きるかは知らない（「つかんだか・つかんでいるか・このティックにダメージを与えたか」を答えるだけで、
// 相手へ伝えるのは CombatResolver）。振りほどき（撃たれた量を数える）は ZombieHealthComponent が attack() へ渡す。
class ZombieAttackComponent : public Component{
public:
	ZombieAttackComponent();

	ZombieAttack& attack(){ return mAttack; }
	const ZombieAttack& attack() const { return mAttack; }

	// 見えている相手が間合いに居て始められるなら、攻撃を始めて true（通り道は捨てる）。始めなければ何もせず false。
	// target: 追う先（nullptr なら始めない）。
	bool tryStart( const ChaseTarget* target );
	// 立って攻撃している間、相手の方を向き、手を伸ばしている段階なら寄る（踏み込みは根の動きで別に足される）。
	void approach( const ChaseTarget* target );
	// 攻撃の段階を進める（生きている間、毎ティック）。立っている体は、つかんでいる間はアニメをゆっくり流し、
	// 噛んだら首を突き出し、終わったら立つアニメへ戻す。
	void update( const ChaseTarget* target );
	// 倒れ始めた・よろけたなどで、攻撃していたらやめる（つかんでいたら離す）。
	void cancel();

	bool isActive() const;          // 攻撃の段階の途中か（伸ばす〜戻しまで）
	// 相手を取りに行っている・つかんでいる・腕を伸ばしている間か（生きている間だけ）。
	// 持ち主は、この間ほかのゾンビに攻撃を始めさせない（TickContext::attacker）。
	bool isAttacking() const;
	bool isHoldingTarget() const;       // 相手をつかんでいるか（生きている間だけ）。つかまれた相手は動けない
	bool grabbedTargetThisTick() const; // 直前の update で相手をつかんだか
	// 直前の update で相手にダメージを与えたか（噛んだ・腕が当たった。量は ZombieAttack::hitDamage）
	bool hitTargetThisTick() const;

	// Component（Transform・SkinnedModel・ZombieState・ZombieChase・ZombieDismember・ZombieRootMotion が要る）
	void start() override;

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
