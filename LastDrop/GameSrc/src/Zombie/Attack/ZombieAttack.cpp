#include "GameSrc/Zombie/Attack/ZombieAttack.h"
#include "GameSrc/Core/GameTime.h"

namespace{
	// ── 立っている体：攻撃のアニメ（atack3）の時刻（フレーム。変換で 0 起点になっている。Blender の 2 フレーム目が 0）──
	const float kStartTime = 0.f; // 攻撃のアニメは頭から再生する
	// 両手を伸ばし切る時刻。ここでつかめるかを決める（Blender で手が一番前へ出た 47 フレーム目）。
	const float kGrabTime = 45.f;
	// 噛む時刻。つかんで止まっている区間（Blender の 56〜92）の終わり近く。この後は元の姿勢へ戻っていく。
	const float kBiteTime = 88.f;
	// つかんでから噛むまでの長さ（秒）。相手が撃って振りほどく時間。アニメのままだと 0.7 秒しか無いので、この区間だけゆっくり流す。
	const float kHoldSeconds = 1.2f;
	const float kHoldPlaybackRate = ( kBiteTime - kGrabTime ) / ( kHoldSeconds * GameTime::kTicksPerSecond );
	const float kNormalPlaybackRate = 1.f;

	// ── 体を引きずる体：アニメの代わりの時計の時刻（フレーム）。2026-11-01〜11-05 の引っかき（atack1）と同じ時間割のまま ──
	// 腕が当たる時刻（始めてから約 0.47 秒。この間に離れれば避けられる）と、腕を戻し終える時刻（約 1 秒）。
	const float kSwingHitTime = 28.f;
	const float kSwingPeriod = 60.f;
	// 時計の 1 ティックに進む時刻（フレーム）。アニメと同じく 1 ティックに 1 フレーム。
	const float kClockStep = 1.f;

	// 1回噛んだ・腕が当たったときのダメージ。満タンから 5 回で死ぬ（PlayerHealth の最大の体力 100 から）。
	const float kHitDamage = 20.f;
	// つかんでいる間にこれだけダメージ（倍率を掛けた後）を受けたら離す。胴なら 2 発、頭なら 1 発（Bullet の 40 × 倍率）。
	const float kShakeOffDamage = 60.f;
	// 攻撃が終わって（噛み終えて・振りほどかれて・倒れかけて）から、次の攻撃を始めるまで（秒）。
	const float kCooldownSeconds = 2.f;
	const int kCooldownTicks = GameTime::secondsToTicks( kCooldownSeconds );
}

ZombieAttack::ZombieAttack() :
	mPhase( Phase::Ready ),
	mCooldownTicks( 0 ),
	mShakeDamage( 0.f ),
	mClock( kStartTime ),
	mGrabbedThisTick( false ),
	mHitThisTick( false ) {
}

bool ZombieAttack::canStart() const {
	return mPhase == Phase::Ready && mCooldownTicks <= 0;
}

void ZombieAttack::start() {
	mPhase = Phase::Reach;
	mShakeDamage = 0.f;
}

void ZombieAttack::startSwing() {
	mPhase = Phase::Swing;
	mClock = kStartTime;
}

void ZombieAttack::updateWithoutAnimation( bool targetInReach ) {
	bool finished = isActive() && mClock >= kSwingPeriod;
	update( mClock, finished, targetInReach );
	if ( isActive() ) {
		mClock += kClockStep;
	}
}

void ZombieAttack::update( float animationTime, bool animationFinished, bool targetInReach ) {
	mGrabbedThisTick = false;
	mHitThisTick = false;
	switch ( mPhase ) {
		case Phase::Ready:
			if ( mCooldownTicks > 0 ) {
				--mCooldownTicks;
			}
			break;
		case Phase::Reach:
			if ( animationTime >= kGrabTime ) {
				// 空振りは、つかんだ姿勢のまま元へ戻るまで流す（噛まない）。その間に相手は離れられる。
				mPhase = targetInReach ? Phase::Hold : Phase::Recover;
				mGrabbedThisTick = targetInReach;
			}
			break;
		case Phase::Hold:
			if ( animationTime >= kBiteTime ) {
				mPhase = Phase::Recover; // 1 回だけ噛んで離す
				mHitThisTick = true;
			}
			break;
		case Phase::Swing:
			if ( animationTime >= kSwingHitTime ) {
				// 空振りでも、伸ばした腕を戻し終えるまで流す。
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

bool ZombieAttack::takeShakeDamage( float amount ) {
	if ( mPhase != Phase::Hold ) {
		return false;
	}
	mShakeDamage += amount;
	if ( mShakeDamage < kShakeOffDamage ) {
		return false;
	}
	finish();
	return true;
}

void ZombieAttack::cancel() {
	if ( mPhase != Phase::Ready ) {
		finish();
	}
}

void ZombieAttack::finish() {
	mPhase = Phase::Ready;
	mCooldownTicks = kCooldownTicks;
	mShakeDamage = 0.f;
}

ZombieAttack::Phase ZombieAttack::phase() const {
	return mPhase;
}

bool ZombieAttack::isActive() const {
	return mPhase != Phase::Ready;
}

bool ZombieAttack::isAttacking() const {
	return mPhase == Phase::Reach || mPhase == Phase::Hold || mPhase == Phase::Swing;
}

bool ZombieAttack::isHolding() const {
	return mPhase == Phase::Hold;
}

bool ZombieAttack::grabbedThisTick() const {
	return mGrabbedThisTick;
}

bool ZombieAttack::hitThisTick() const {
	return mHitThisTick;
}

float ZombieAttack::playbackRate() const {
	return ( mPhase == Phase::Hold ) ? kHoldPlaybackRate : kNormalPlaybackRate;
}

float ZombieAttack::hitDamage() {
	return kHitDamage;
}

float ZombieAttack::startTime() {
	return kStartTime;
}
