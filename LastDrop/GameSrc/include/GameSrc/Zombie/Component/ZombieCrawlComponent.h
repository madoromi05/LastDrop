#ifndef INCLUDED_ZOMBIE_CRAWL_COMPONENT_H
#define INCLUDED_ZOMBIE_CRAWL_COMPONENT_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/Crawl/ZombieCrawler.h"

class ZombieModel;
class HitWorld;
class SkinnedMeshInstance;
class CharacterMover;
class ZombieStateComponent;
class ZombieAttackComponent;
class ZombieLocomotion;
class ZombieRootMotionSpan;
class ZombieAnimationSet;
struct ChaseTarget;

// 脚を失って倒れた後（Downed）、倒れ終わったら腕で体を引きずって追ってくる（SummaryPlan/個別仕様/ZombieCrawl_Plan.txt）。
// 引きずる腕・胸の置き場所は ZombieCrawler（ZombieDismemberComponent が持つ。残っている腕しか使えないため）、ここはいつ・どちらへ引くかを決める。
// 追いついたら、立っている体と同じく ZombieAttackComponent で攻撃を始める（気付く前はうろつかず、その場でもがくだけ）。
class ZombieCrawlComponent : public Component{
public:
	ZombieCrawlComponent();

	// Downed の間、毎ティック。倒れ終わるまでは何もせず、倒れ終わったら引きずり始め、追う先へ向けて腕で体を引く。
	// target: 追う先（nullptr なら追わない）。
	void update( const ChaseTarget* target, const HitWorld& hitWorld );

	// Component（ZombieModel・Transform・SkinnedModel・CharacterCollider・ZombieDismember・ZombieState・ZombieChase・
	// ZombieRootMotion が要る。ZombieAttack は任意＝付いていなければ、追いついても攻撃せずにもがくだけ）
	void start() override;

private:
	bool canStart() const; // 前へ倒れるアニメで倒れ終わったか（引きずれる骨がそろっているか）
	void startCrawling();
	// 追う先を考えて、腕をどう動かすか（もがく・引く・止める）を決める。追いついたら攻撃を始める。moving: 進むか（向き終わったか）。
	// attack: 攻撃の部品（外されていれば nullptr）。
	ZombieCrawler::Effort chase( const ChaseTarget* target, const HitWorld& hitWorld, ZombieAttackComponent* attack, bool* moving );

	// ── 同じ物の部品（start で借りる）──
	const ZombieModel* mModel;         // 引きずる腕の骨がそろっているかを聞く設計図（ZombieModelComponent）
	GameLib::Math::Vector3* mPosition; // 足元の位置（TransformComponent）
	float* mAngleY;                    // 向き（度。TransformComponent）
	SkinnedMeshInstance* mMesh;
	const CharacterMover* mMover;      // 壁・車に当たったら胸の位置で沿って滑る
	ZombieCrawler* mCrawler;
	const ZombieStateComponent* mState;
	ZombieLocomotion* mLocomotion;
	ZombieRootMotionSpan* mRootSpan;
	ZombieAnimationSet* mAnimations;
};

#endif
