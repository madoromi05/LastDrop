#ifndef INCLUDED_ZOMBIE_CHASE_COMPONENT_H
#define INCLUDED_ZOMBIE_CHASE_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/ZombieLocomotion.h"
#include "GameSrc/Zombie/ZombieSway.h"
#include <memory>

class HitWorld;
class ZombieCrowd;
class SkinnedMeshInstance;
class ZombieStateComponent;
class ZombieAnimationSet;
struct ChaseTarget;

// ゾンビがどこへ行き・どちらを向いて・どう進むか（ZombieLocomotion。中で ZombieChase が行き先を決め、ZombiePath が建物を回り込む
// 通り道を出す。SummaryPlan/個別仕様/ZombieChase_Plan.txt・ZombiePath_Plan.txt）と、近づくと胴を左右に揺らすフラつき（ZombieSway）。
// 立っている体が考えて（見て・行き先を決め・向きを変え）歩く・追いついたら横へよける、と揺れの強さを決めるのもここ。
// 歩く・滑るのは同じ物の CharacterColliderComponent の移動を借りる。攻撃のことは知らない
// （「考える → 攻撃を始めるか → 歩く」の順に呼ぶのは ZombieBrainComponent。SummaryPlan/個別仕様/ZombieBrainSplit_Plan.txt）。
class ZombieChaseComponent : public Component{
public:
	// sightPhase: 見通し・まっすぐ通れるかを調べる時刻をずらす番号（何体目か）。wanderSeed: うろつく先・立つ長さを選ぶ乱数の種。
	// swayPeriodSeconds / swayStartPhase: 揺れの周期・始めの位相（ZombieSway）。
	// 建物を回り込む通り道を聞く町の床の形（NavMesh）は共有の棚から、フラつきの骨の設計図は ZombieModelComponent から、awake で引く。
	ZombieChaseComponent( int sightPhase, int wanderSeed, float swayPeriodSeconds, float swayStartPhase );
	~ZombieChaseComponent();

	ZombieLocomotion& locomotion();
	const ZombieLocomotion& locomotion() const;
	ZombieSway& sway();

	// 立っている体が、追う先を見て行き先を決め、向きを変える（見た目の向きも合わせる）。歩くかどうかを返す。
	// target: 追う先（nullptr なら追わない）。crowd: 近くの仲間（nullptr なら避けない）。
	ZombieLocomotion::Intent think( const ChaseTarget* target, const HitWorld& hitWorld, const ZombieCrowd* crowd );
	// think の結果どおりに歩く（向き終わっていれば進む）。追いついて立ち止まっていれば、仲間の後ろから横へよける。
	void walk( const ZombieLocomotion::Intent& intent, const ZombieCrowd* crowd );
	// このティックの揺れを決める（姿勢を求める前に呼ぶ）。pursuing: 立って追える体で、よろけ・攻撃の最中でないか。
	void updateSway( const ChaseTarget* target, bool pursuing );

	// ── 確認用の表示が読む ──
	ZombieChase::Mode mode() const;                       // 追ってくる AI の今の考え（Wander / Chase / Search）
	const ZombiePath& path() const;                       // 建物を回り込む通り道
	const GameLib::Math::Vector3& separation() const;     // 最後に歩く向きへ混ぜた、仲間から離れる向き

	// Component（ZombieModel・Transform・SkinnedModel・CharacterCollider・ZombieState・ZombieRootMotion が要る）
	// awake：滑る移動を借りて ZombieLocomotion を、フラつきの骨の設計図を借りて ZombieSway を作る
	virtual void awake();
	virtual void start();

private:
	// 今揺らしたい強さ（0〜1）。立って自キャラを追っていて、近いほど強い。それ以外は 0。
	float swayStrength( const ChaseTarget* target, bool pursuing ) const;

	int mSightPhase;
	int mWanderSeed;
	float mSwayPeriodSeconds;
	float mSwayStartPhase;
	std::unique_ptr< ZombieSway > mSway;             // 設計図を借りるので awake で作る
	std::unique_ptr< ZombieLocomotion > mLocomotion; // 滑る移動を借りるので awake で作る
	// ── 同じ物の部品（start で借りる）──
	GameLib::Math::Vector3* mPosition; // 足元の位置（TransformComponent）
	float* mAngleY;                    // 向き（度。TransformComponent）
	SkinnedMeshInstance* mMesh;
	const ZombieStateComponent* mState;
	ZombieAnimationSet* mAnimations;
};

#endif
