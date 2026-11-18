#include "GameSrc/Zombie/Move/ZombieMoveTrace.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/AngleMath.h"

using namespace GameLib::Math;

namespace{
	const float kWindowSeconds = 1.f;								// 確かめる長さ（秒。歩きたいティックが続いた長さ）
	const int kWindowTicks = GameTime::secondsToTicks( kWindowSeconds );
	const float kMarkSeconds = 3.f;									// 引っかかった後、画面に印を出す長さ（秒）
	const int kMarkTicks = GameTime::secondsToTicks( kMarkSeconds );
	const float kMinWanted = 0.3f;									// 1 秒に進みたかった長さがこれより短ければ「進めない」を確かめない（m。ほとんど止まっている）
	const float kMinForwardShare = 0.3f;							// 進みたかった長さのうち、体の前へ進めた割合がこれより低ければ「進めない」
	const float kTurningShare = 0.5f;								// 歩きたいティックのうち、その場で回っていた割合がこれ以上なら「回り続ける」
}

ZombieMoveTrace::ZombieMoveTrace() :
	mMarkTicks( 0 ),
	mAimPoint( 0.f, 0.f, 0.f ),
	mWalkingTicks( 0 ),
	mTurningTicks( 0 ),
	mWanted( 0.f ),
	mForward( 0.f ) {
}

void ZombieMoveTrace::beginTick() {
	if ( mMarkTicks > 0 ) {
		--mMarkTicks;
	}
}

void ZombieMoveTrace::mark() {
	mMarkTicks = kMarkTicks;
}

void ZombieMoveTrace::setAimPoint( const Vector3& point ) {
	mAimPoint = point;
}

void ZombieMoveTrace::noteIntent( bool walking, bool facing ) {
	if ( !walking ) {
		resetWindow(); // 自分から止まった（着いた・追いついた）。止まる前の分で確かめない
		return;
	}
	++mWalkingTicks;
	if ( !facing ) {
		++mTurningTicks;
	}
	if ( mWalkingTicks >= kWindowTicks ) {
		checkWindow();
	}
}

void ZombieMoveTrace::noteStep( float wanted, const Vector3& moved, float angleY ) {
	mWanted += wanted;
	mForward += moved.dot( AngleMath::forwardOf( angleY ) );
}

void ZombieMoveTrace::checkWindow() {
	bool turning = mTurningTicks >= kTurningShare * mWalkingTicks;
	bool noProgress = mWanted >= kMinWanted && mForward < kMinForwardShare * mWanted;
	if ( turning || noProgress ) {
		mark();
	}
	resetWindow();
}

void ZombieMoveTrace::resetWindow() {
	mWalkingTicks = 0;
	mTurningTicks = 0;
	mWanted = 0.f;
	mForward = 0.f;
}

bool ZombieMoveTrace::isMarked() const {
	return mMarkTicks > 0;
}

const Vector3& ZombieMoveTrace::aimPoint() const {
	return mAimPoint;
}
