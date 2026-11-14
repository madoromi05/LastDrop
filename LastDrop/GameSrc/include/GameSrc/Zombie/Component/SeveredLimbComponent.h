#ifndef INCLUDED_SEVERED_LIMB_COMPONENT_H
#define INCLUDED_SEVERED_LIMB_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/Damage/SeveredLimb.h"

// ゾンビから千切れた部位 1 つの物（腕・脚・頭）の部品（SummaryPlan/個別仕様/設計と分割/SeveredLimbObject_Plan.txt）
// 飛ぶ・地面で止まる・沈む動きと見た目は SeveredLimb に任せる。ここは、いつ進めるか（Late の段）と、物の寿命を受け持つ
// 沈み終えたら、自分の物に消す印を付ける（物の寿命は物が決める。ZombieCorpseComponent と同じ形）
// 落ちる先の地面は共有の棚から引く（町の地面だけ。建物・車の上には乗らず、抜ける）
// 置き場所・見た目・動きを別々の部品には分けない（飛び方と描く箱が同じ中心を使う 1 つの仕事のため）
class SeveredLimbComponent : public Component{
public:
	// 引数は SeveredLimb のコンストラクタと同じ
	SeveredLimbComponent( const SkinnedMeshData* data, const ZombieSeverLayout::Cut& cut, const std::vector< GameLib::Math::Matrix34 >& worldPose,
		const GameLib::Math::Vector3& direction, std::mt19937* random );

	void draw( const DrawCulling& culling ) const;	// culling で描くと決まった時だけ描く（描く側が、全部の SeveredLimbComponent をたどって呼ぶ）

	// Component（落ちる先の地面は共有の棚から引く）
	UpdatePhase updatePhase() const override;
	void start() override;
	void update() override;

private:
	SeveredLimb mLimb;
	const IRayBlocker* mGround;	// 借りているだけ（共有の棚の町の地面）
};

#endif
