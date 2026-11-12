#include "GameSrc/Zombie/ZombieUncertaintyParams.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Random.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 丈夫さ・フラつきの値（F6 で動かせる。湧いた体が作られるときに読む）
	ZombieUncertaintyParams gUncertaintyParams;

	// low〜high から一様に選ぶ。F6 で幅を逆にしても（high <= low）、low を使う
	float pickInRange( float low, float high, Random* random ) {
		ASSERT( random && "ZombieUncertaintyParams : random is null." );
		if ( high <= low ) {
			return low;
		}
		return random->getFloat( low, high );
	}
}

float ZombieUncertaintyParams::pickToughness( Random* random ) const {
	return pickInRange( toughnessMin, toughnessMax, random );
}

float ZombieUncertaintyParams::pickSwayPeriod( Random* random ) const {
	return pickInRange( swayPeriodMin, swayPeriodMax, random );
}

ZombieUncertaintyParams& zombieUncertaintyParams() {
	return gUncertaintyParams;
}
