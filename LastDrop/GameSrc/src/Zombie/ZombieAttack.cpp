#include "GameSrc/Zombie/ZombieAttack.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	// ── 攻撃のアニメ（atack1）の時刻（フレーム。変換で 0 起点になっている。Blender の 2 フレーム目が 0）──
	const float kStartTime = 0.f; // 攻撃のアニメは頭から再生する
	// 振り下ろした手が当たる時刻。Blender で右手が一番前（足元から 0.86m）へ来た 30 フレーム目。
	// 16〜18 フレーム目で振り上げているので、始めてから当たるまで約 0.47 秒（この間に下がれば避けられる）。
	const float kHitTime = 28.f;
	// 時計で進める体（体を引きずるゾンビ）の、1 ティックに進む時刻（フレーム）。アニメと同じく 1 ティックに 1 フレーム。
	const float kClockStep = 1.f;

	// 1回当たったときのダメージ。満タンから 5 回当たると死ぬ（PlayerHealth の最大の体力 100 から）。
	const float kHitDamage = 20.f;
	// 攻撃が終わって（振り終えて・倒れかけて）から、次の攻撃を始めるまで（秒）。
	const float kCooldownSeconds = 2.f;
	const int kCooldownTicks = GameTime::secondsToTicks( kCooldownSeconds );
}

ZombieAttack::ZombieAttack() :
	mPhase( Phase::Ready ),
	mCooldownTicks( 0 ),
	mClock( kStartTime ),
	mHitThisTick( false ) {
}

bool ZombieAttack::canStart() const {
	return mPhase == Phase::Ready && mCooldownTicks <= 0;
}

void ZombieAttack::start() {
	mPhase = Phase::Swing;
	mClock = kStartTime;
}

void ZombieAttack::updateWithoutAnimation( float animationPeriod, bool targetInReach ) {
	bool finished = isActive() && mClock >= animationPeriod;
	update( mClock, finished, targetInReach );
	if ( isActive() ) {
		mClock += kClockStep;
	}
}

void ZombieAttack::update( float animationTime, bool animationFinished, bool targetInReach ) {
	mHitThisTick = false;
	switch ( mPhase ) {
		case Phase::Ready:
			if ( mCooldownTicks > 0 ) {
				--mCooldownTicks;
			}
			break;
		case Phase::Swing:
			if ( animationTime >= kHitTime ) {
				// 空振りでも、振り終えた姿勢のまま元へ戻るまで流す。
				mPhase = Phase::Recover;
				mHitThisTick = targetInReach;
			}
			break;
		case Phase::Recover:
			if ( animationFinished ) {
				finish();
			}
			break;
	}
}

void ZombieAttack::cancel() {
	if ( mPhase != Phase::Ready ) {
		finish();
	}
}

void ZombieAttack::finish() {
	mPhase = Phase::Ready;
	mCooldownTicks = kCooldownTicks;
}

ZombieAttack::Phase ZombieAttack::phase() const {
	return mPhase;
}

bool ZombieAttack::isActive() const {
	return mPhase != Phase::Ready;
}

bool ZombieAttack::isAttacking() const {
	return mPhase == Phase::Swing;
}

bool ZombieAttack::hitThisTick() const {
	return mHitThisTick;
}

float ZombieAttack::hitDamage() {
	return kHitDamage;
}

float ZombieAttack::startTime() {
	return kStartTime;
}
