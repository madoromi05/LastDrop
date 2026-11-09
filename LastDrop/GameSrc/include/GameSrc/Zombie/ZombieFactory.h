#ifndef INCLUDED_ZOMBIE_FACTORY_H
#define INCLUDED_ZOMBIE_FACTORY_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Zombie/Animation/ZombieAnimationSet.h"

class ZombieModel;
class ZombieBrainComponent;
class GameObjectWorld;

// ゾンビ 1 体を部品から組み立てる（Unity のプレハブの代わり。SummaryPlan/個別仕様/設計と分割/ComponentSystem2_Plan.txt の 4 章）。
// 部品の並びと、体ごとの個体差（丈夫さ・揺れの速さ・うろつき方の乱数・走る体か）を 1 か所で決める。
// 付ける順は自由（部品は awake で自分の物を作り、start で隣の部品を借りる。SummaryPlan/個別仕様/設計と分割/ComponentSystem3_Plan.txt）。
// 世界に 1 つの物（当たり判定の名簿・町の床の形・ティックの周り）は、部品が GameObjectWorld の共有の棚から引く
// （spawn する world に、持ち主の GameWorld が置いておくこと）。
class ZombieFactory{
public:
	// model: 形・アニメと当たり判定の設計図。所有権は受け取らない。nullptr 不可。作ったゾンビより長く生きていること（GameWorld が持つ）。
	explicit ZombieFactory( const ZombieModel* model );

	// ゾンビを組み立てて world へ spawn し、つなぐ部品（ZombieBrainComponent）を返す。
	// position / angleY: 足元の位置と向き（度。0 は +Z）。names: 使うアニメ（move が nullptr なら T ポーズの確認用。丈夫さは表のまま）。
	// sightPhase: 見通しを調べる時刻をずらす番号（何体目か）。randomSeed: 体ごとの乱数の種（体ごとに違う値）。
	ZombieBrainComponent* spawn( GameObjectWorld* world, const GameLib::Math::Vector3& position, float angleY,
		const ZombieAnimationNames& names, int sightPhase, int randomSeed ) const;

private:
	const ZombieModel* mModel;
};

#endif
