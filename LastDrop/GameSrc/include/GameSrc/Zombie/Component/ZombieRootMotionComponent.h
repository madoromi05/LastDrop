#ifndef INCLUDED_ZOMBIE_ROOT_MOTION_COMPONENT_H
#define INCLUDED_ZOMBIE_ROOT_MOTION_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/Animation/ZombieRootMotionSpan.h"
#include "GameSrc/Zombie/Animation/ZombieAnimationSet.h"
#include <memory>

// ゾンビの使うアニメと、どのアニメをどう繋ぎ・どの根の動きで流すか（ZombieAnimationSet）と、攻撃・よろけ・倒れる間に
// 腰の動きを置き場所へ移す区間（ZombieRootMotionSpan。SummaryPlan/個別仕様/ゾンビ/ZombieRootMotion_Plan.txt）
// 区間は見た目と滑る移動を借りるので、同じ物の SkinnedModelComponent・CharacterColliderComponent を awake で借りて作る
// （どちらも付けたときから在る物。作った区間・アニメの組は、隣の部品が start から借りる）
// いつ区間を始め・閉じるかは ZombieBrainComponent が決める
class ZombieRootMotionComponent : public Component{
public:
	// names: 使うアニメの名前（文字列は持ち主の定数。awake の中で引くだけで、覚えない）
	explicit ZombieRootMotionComponent( const ZombieAnimationNames& names );
	~ZombieRootMotionComponent();

	ZombieRootMotionSpan& span();
	const ZombieRootMotionSpan& span() const;
	ZombieAnimationSet& animations();
	const ZombieAnimationSet& animations() const;

	// Component（ZombieModelComponent・SkinnedModelComponent・CharacterColliderComponent が要る）
	void awake() override;

private:
	ZombieAnimationNames mNames;
	std::unique_ptr< ZombieRootMotionSpan > mSpan;      // 見た目と滑る移動を借りるので awake で作る
	std::unique_ptr< ZombieAnimationSet > mAnimations;  // mSpan を借りるので、その後に作る
};

#endif
