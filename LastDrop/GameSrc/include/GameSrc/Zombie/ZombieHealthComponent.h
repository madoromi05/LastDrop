#ifndef INCLUDED_ZOMBIE_HEALTH_COMPONENT_H
#define INCLUDED_ZOMBIE_HEALTH_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/ZombieHealth.h"
#include "GameSrc/Zombie/ZombieFlinch.h"
#include "GameSrc/Zombie/ZombieStagger.h"

// ゾンビの体力と、撃たれたときの体の反応（1 発ごとののけぞり ZombieFlinch・ときどきのよろけ ZombieStagger）。
// 体力の最大は体ごとに違う（作るときに丈夫さを選ぶ。SummaryPlan/個別仕様/ZombieUncertainty_Plan.txt）。
// 撃たれて何をするか（倒れる・よろける）を選んで動かすのは ZombieBrainComponent。ここは数と時間を持つだけ。
class ZombieHealthComponent : public Component{
public:
	// toughness: 丈夫さ（体力の最大に掛ける）。flinchLayout: のけぞる骨の設計図（ZombieModel の物を借りる。nullptr 不可）。
	ZombieHealthComponent( float toughness, const ZombieFlinchLayout* flinchLayout ) :
		mHealth( toughness ),
		mFlinch( flinchLayout ){}

	ZombieHealth& health(){ return mHealth; }
	const ZombieHealth& health() const { return mHealth; }
	ZombieFlinch& flinch(){ return mFlinch; }
	ZombieStagger& stagger(){ return mStagger; }
	// よろけのアニメの最中か（生きている間だけ true になりうる）。確認用の表示に使う。
	bool isStaggering() const { return mStagger.isActive(); }

private:
	ZombieHealth mHealth;
	ZombieFlinch mFlinch;
	ZombieStagger mStagger; // よろけの最中か・次のよろけまでの間あけ
};

#endif
