#ifndef INCLUDED_ZOMBIE_CORPSE_COMPONENT_H
#define INCLUDED_ZOMBIE_CORPSE_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/Zombie/ZombieCorpse.h"

// 倒れ終わってから、死体として残り、地面へ沈んで消えるまでの時間（ZombieCorpse）。
// いつ数え始めるか・沈む分だけ見た目を下げるのは ZombieBrainComponent。
class ZombieCorpseComponent : public Component{
public:
	ZombieCorpse& corpse(){ return mCorpse; }
	const ZombieCorpse& corpse() const { return mCorpse; }

private:
	ZombieCorpse mCorpse;
};

#endif
