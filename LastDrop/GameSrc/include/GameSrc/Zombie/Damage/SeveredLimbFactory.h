#ifndef INCLUDED_SEVERED_LIMB_FACTORY_H
#define INCLUDED_SEVERED_LIMB_FACTORY_H

#include "GameSrc/Zombie/Damage/ZombieSeverLayout.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"
#include <random>
#include <vector>

class SkinnedMeshData;
class GameObject;
class GameObjectWorld;

// 千切れた部位の物を部品から組み立てる（Unity のプレハブの代わり。SummaryPlan/個別仕様/設計と分割/SeveredLimbObject_Plan.txt）
// 同時に出しておける数の上限と、回る軸と速さのばらつきの乱数を持つ
// 上限を超える分は、古い物から消す（1 つにつきゾンビ 1 体ぶんの頂点を変形するため）
// どこで出すか（ゾンビの部位が 0 になったら）は頼む側（CombatResolver）が決める
class SeveredLimbFactory{
public:
	SeveredLimbFactory();
	SeveredLimbFactory( const SeveredLimbFactory& ) = delete;
	SeveredLimbFactory& operator=( const SeveredLimbFactory& ) = delete;

	// 千切れた部位の物を world へ spawn して返す。引数は SeveredLimb のコンストラクタと同じ（乱数はここが持つ物を使う）
	// world: nullptr 不可。物の並びをたどっている最中には呼ばない（spawn するため）
	GameObject* spawn( GameObjectWorld* world, const SkinnedMeshData* data, const ZombieSeverLayout::Cut& cut,
		const std::vector< GameLib::Math::Matrix34 >& worldPose, const GameLib::Math::Vector3& direction );

private:
	// これから 1 つ出すと上限を超えるなら、超える数だけ、古い物から消す印を付ける（消えるのは次の flush）
	// 印の付いた物も数に入るので、同じティックに続けて出しても、flush の後は上限に収まる
	void retireOldest( GameObjectWorld* world ) const;

	std::mt19937 mRandom; // 回る軸と速さのばらつき
};

#endif
