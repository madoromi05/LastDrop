#ifndef INCLUDED_ZOMBIE_CHASE_COMPONENT_H
#define INCLUDED_ZOMBIE_CHASE_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/ZombieLocomotion.h"
#include "GameSrc/Zombie/ZombieSway.h"
#include <memory>

class NavMesh;

// ゾンビがどこへ行き・どちらを向いて・どう進むか（ZombieLocomotion。中で ZombieChase が行き先を決め、ZombiePath が建物を回り込む
// 通り道を出す。SummaryPlan/個別仕様/ZombieChase_Plan.txt・ZombiePath_Plan.txt）と、近づくと胴を左右に揺らすフラつき（ZombieSway）。
// 歩く・滑るのは同じ物の CharacterColliderComponent の移動を借りる。いつ考えて歩くかは ZombieBrainComponent が決める。
class ZombieChaseComponent : public Component{
public:
	// navMesh: 建物を回り込む通り道を聞く町の床の形。所有権は受け取らない。nullptr 不可。この部品より長く生きていること。
	// sightPhase: 見通し・まっすぐ通れるかを調べる時刻をずらす番号（何体目か）。wanderSeed: うろつく先・立つ長さを選ぶ乱数の種。
	// swayLayout / swayPeriodSeconds / swayStartPhase: フラつきの骨の設計図（ZombieModel の物）・揺れの周期・始めの位相（ZombieSway）。
	ZombieChaseComponent( const NavMesh* navMesh, int sightPhase, int wanderSeed,
		const ZombieFlinchLayout* swayLayout, float swayPeriodSeconds, float swayStartPhase );
	~ZombieChaseComponent();

	ZombieLocomotion& locomotion();
	const ZombieLocomotion& locomotion() const;
	ZombieSway& sway();

	// ── 確認用の表示が読む ──
	ZombieChase::Mode mode() const;                       // 追ってくる AI の今の考え（Wander / Chase / Search）
	const ZombiePath& path() const;                       // 建物を回り込む通り道
	const GameLib::Math::Vector3& separation() const;     // 最後に歩く向きへ混ぜた、仲間から離れる向き

	// Component（CharacterColliderComponent が要る。滑る移動を借りて ZombieLocomotion を作る）
	virtual void start();

private:
	const NavMesh* mNavMesh;
	int mSightPhase;
	int mWanderSeed;
	ZombieSway mSway;
	std::unique_ptr< ZombieLocomotion > mLocomotion; // 滑る移動を借りるので start で作る
};

#endif
