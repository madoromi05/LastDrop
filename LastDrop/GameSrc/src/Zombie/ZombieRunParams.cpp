#include "GameSrc/Zombie/ZombieRunParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Random.h"

namespace{
	// 走るゾンビの値（F6 で動かせる）。
	ZombieRunParams gRunParams;
}

bool ZombieRunParams::pickRunner( GameLib::Math::Random* random ) const {
	ASSERT( random && "ZombieRunParams : random is null." );
	return random->getFloat() < runnerRatio;
}

ZombieRunParams& zombieRunParams() {
	return gRunParams;
}
