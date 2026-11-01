#ifndef INCLUDED_ZOMBIE_ROOT_MOTION_COMPONENT_H
#define INCLUDED_ZOMBIE_ROOT_MOTION_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/ZombieRootMotionSpan.h"
#include "GameSrc/Zombie/ZombieAnimationSet.h"
#include <memory>

class ZombieModel;

// ゾンビの使うアニメと、どのアニメをどう繋ぎ・どの根の動きで流すか（ZombieAnimationSet）と、攻撃・よろけ・倒れる間に
// 腰の動きを置き場所へ移す区間（ZombieRootMotionSpan。SummaryPlan/個別仕様/ZombieRootMotion_Plan.txt）。
// 区間は見た目と滑る移動を借りるので、同じ物の SkinnedModelComponent・CharacterColliderComponent を start で借りて作る。
// いつ区間を始め・閉じるかは ZombieBrainComponent が決める。
class ZombieRootMotionComponent : public Component{
public:
	// model: 形・アニメと根の動きの表（ZombieModel）。所有権は受け取らない。nullptr 不可。この部品より長く生きていること。
	// names: 使うアニメの名前（文字列は持ち主の定数。start の中で引くだけで、覚えない）。
	ZombieRootMotionComponent( const ZombieModel* model, const ZombieAnimationNames& names );
	~ZombieRootMotionComponent();

	ZombieRootMotionSpan& span();
	const ZombieRootMotionSpan& span() const;
	ZombieAnimationSet& animations();
	const ZombieAnimationSet& animations() const;

	// Component（SkinnedModelComponent・CharacterColliderComponent が要る）
	virtual void start();

private:
	const ZombieModel* mModel;
	ZombieAnimationNames mNames;
	std::unique_ptr< ZombieRootMotionSpan > mSpan;      // 見た目と滑る移動を借りるので start で作る
	std::unique_ptr< ZombieAnimationSet > mAnimations;  // mSpan を借りるので、その後に作る
};

#endif
