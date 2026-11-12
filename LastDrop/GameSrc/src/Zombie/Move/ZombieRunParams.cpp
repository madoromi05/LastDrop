#include "GameSrc/Zombie/Move/ZombieRunParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Random.h"

namespace{
	ZombieRunParams gRunParams;	// 走るゾンビの値（F6 で動かせる）
}

bool ZombieRunParams::pickRunner( GameLib::Math::Random* random ) const {
	ASSERT( random && "ZombieRunParams : random is null." );
	return random->getFloat() < runnerRatio;
}

ZombieRunParams& zombieRunParams() {
	return gRunParams;
}
