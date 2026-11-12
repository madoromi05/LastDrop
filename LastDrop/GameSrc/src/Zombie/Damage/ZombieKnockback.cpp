#include "GameSrc/Zombie/Damage/ZombieKnockback.h"
#include "GameSrc/Zombie/Damage/ZombieStaggerParams.h"
#include "GameSrc/Core/GameTime.h"

using GameLib::Math::Vector3;

namespace{
	// 押し終えるまでの最短（ティック）。秒が短すぎても 0 で割らないように
	const int kMinTicks = 1;

	ZombieStaggerParams gParams;

	// 押し始めてから t（0〜1）まで進んだときに、押し終えた割合（0〜1）。初めが速く、だんだん遅くなる（2 次の ease-out）
	float pushedFraction( float t ) {
		float rest = 1.f - t;
		return 1.f - rest * rest;
	}
}

ZombieStaggerParams& zombieStaggerParams() {
	return gParams;
}

ZombieKnockback::ZombieKnockback() :
	mDirection( 0.f, 0.f, 0.f ),
	mDistance( 0.f ),
	mTotalTicks( kMinTicks ),
	mElapsedTicks( kMinTicks ) {
}

void ZombieKnockback::start( const Vector3& direction ) {
	Vector3 flat( direction.x, 0.f, direction.z );
	float length = flat.length();
	if ( length <= 0.f ) {
		cancel();
		return;
	}
	flat *= 1.f / length;
	mDirection = flat;
	mDistance = gParams.knockbackDistance;
	int ticks = GameTime::secondsToTicks( gParams.knockbackSeconds );
	mTotalTicks = ( ticks > kMinTicks ) ? ticks : kMinTicks;
	mElapsedTicks = 0;
}

void ZombieKnockback::cancel() {
	mElapsedTicks = mTotalTicks;
}

Vector3 ZombieKnockback::step() {
	if ( !isActive() ) {
		return Vector3( 0.f, 0.f, 0.f );
	}
	float total = static_cast< float >( mTotalTicks );
	float before = pushedFraction( static_cast< float >( mElapsedTicks ) / total );
	++mElapsedTicks;
	float after = pushedFraction( static_cast< float >( mElapsedTicks ) / total );
	Vector3 pushed;
	pushed.setMul( mDirection, mDistance * ( after - before ) );
	return pushed;
}

bool ZombieKnockback::isActive() const {
	return mElapsedTicks < mTotalTicks;
}
