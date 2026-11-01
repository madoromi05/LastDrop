#ifndef INCLUDED_ZOMBIE_FACTORY_H
#define INCLUDED_ZOMBIE_FACTORY_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Zombie/ZombieAnimationSet.h"
#include "GameSrc/Zombie/ZombieBrainComponent.h"

class ZombieModel;
class CollisionWorld;
class NavMesh;
class GameObjectWorld;

// ゾンビ 1 体を部品から組み立てる（Unity のプレハブの代わり。SummaryPlan/個別仕様/ComponentSystem2_Plan.txt の 4 章）。
// 部品の並び（付ける順）と、体ごとの個体差（丈夫さ・揺れの速さ・うろつき方の乱数）を 1 か所で決める。
// 付ける順に意味がある：滑る移動（CharacterCollider）を借りる部品（ZombieChase・ZombieRootMotion）より先に CharacterCollider を、
// 隣の部品を全部使う ZombieBrain を最後に付ける（start は付けた順に呼ばれる）。
class ZombieFactory{
public:
	// model: 形・アニメと当たり判定の設計図。collisionWorld: 歩くときに壁・他のキャラを聞く名簿。navMesh: 建物を回り込む通り道を聞く床の形。
	// context: 全員の Think の段で共有する、このティックの周り。
	// どれも所有権は受け取らない。nullptr 不可。作ったゾンビより長く生きていること（GameWorld が持つ）。
	ZombieFactory( const ZombieModel* model, const CollisionWorld* collisionWorld, const NavMesh* navMesh,
		ZombieBrainComponent::TickContext* context );

	// ゾンビを組み立てて world へ spawn し、つなぐ部品（ZombieBrainComponent）を返す。
	// position / angleY: 足元の位置と向き（度。0 は +Z）。names: 使うアニメ（move が nullptr なら T ポーズの確認用。丈夫さは表のまま）。
	// sightPhase: 見通しを調べる時刻をずらす番号（何体目か）。randomSeed: 体ごとの乱数の種（体ごとに違う値）。
	ZombieBrainComponent* spawn( GameObjectWorld* world, const GameLib::Math::Vector3& position, float angleY,
		const ZombieAnimationNames& names, int sightPhase, int randomSeed ) const;

private:
	const ZombieModel* mModel;
	const CollisionWorld* mCollisionWorld;
	const NavMesh* mNavMesh;
	ZombieBrainComponent::TickContext* mContext;
};

#endif
