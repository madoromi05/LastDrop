#include "GameSrc/Zombie/Damage/ZombieStagger.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	// よろけが終わってから、次のよろけを出さない時間（秒）。頭に当たったときは待たない。
	const float kCooldownSeconds = 1.f;
	const int kCooldownTicks = GameTime::secondsToTicks( kCooldownSeconds );
}

ZombieStagger::ZombieStagger() :
	mActive( false ),
	mCooldownTicks( 0 ) {
}

bool ZombieStagger::canStart( BodyPart part ) const {
	if ( mActive ) {
		return false; // よろけの最中は出し直さない（のけぞりだけ足す）
	}
	return mCooldownTicks <= 0 || part == BodyPart::Head;
}

void ZombieStagger::start() {
	mActive = true;
}

void ZombieStagger::finish() {
	mActive = false;
	mCooldownTicks = kCooldownTicks;
}

void ZombieStagger::cancel() {
	mActive = false;
}

void ZombieStagger::tick() {
	if ( !mActive && mCooldownTicks > 0 ) {
		--mCooldownTicks;
	}
}

bool ZombieStagger::isActive() const {
	return mActive;
}
