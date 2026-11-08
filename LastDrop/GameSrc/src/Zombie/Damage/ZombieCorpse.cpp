#include "GameSrc/Zombie/Damage/ZombieCorpse.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// 倒れ終わってから沈み始めるまで（秒）。
	const float kCorpseSeconds = 10.f;
	// 沈むのにかける時間（秒）と深さ（m）。体の厚み（倒れた胴の高さ）より深くして、沈み終わりに何も見えないようにする。
	const float kSinkSeconds = 1.5f;
	const float kSinkDepth = 0.5f;

	const int kCorpseTicks = GameTime::secondsToTicks( kCorpseSeconds );
	const int kSinkTicks = GameTime::secondsToTicks( kSinkSeconds );
	const int kGoneTicks = kCorpseTicks + kSinkTicks; // 倒れ終わってから消えるまで
}

ZombieCorpse::ZombieCorpse() :
	mTicks( 0 ) {
}

void ZombieCorpse::start() {
	mTicks = 0;
}

void ZombieCorpse::tick() {
	++mTicks;
}

bool ZombieCorpse::isSinking() const {
	return mTicks >= kCorpseTicks;
}

float ZombieCorpse::sinkDepth() const {
	if ( !isSinking() ) {
		return 0.f;
	}
	float progress = static_cast< float >( mTicks - kCorpseTicks ) / static_cast< float >( kSinkTicks );
	return kSinkDepth * Math::min( progress, 1.f );
}

bool ZombieCorpse::isGone() const {
	return mTicks >= kGoneTicks;
}

float ZombieCorpse::secondsUntilGone() const {
	return GameTime::ticksToSeconds( kGoneTicks - mTicks );
}
