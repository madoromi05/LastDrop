#include "GameSrc/Weapon/WeaponReload.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// ── アニメ（reloading.fbx。元は 30fps・3.3 秒。player.json の game_fps でゲームのフレームに直してある）──
	// 使うのは頭から 2.5 秒（左手がハンドガードへ戻った所）まで。その後は構えたまま立っているだけなので見せない。
	// 2 秒で込め終える（バイオハザード RE:4 のライフル程度。撃てない時間が長いほど、弾が切れたときの焦りが出る）よう、1.25 倍で流す。
	const float kAnimationUsedSeconds = 2.5f;
	const float kDurationSeconds = 2.f;
	const float kAnimationRate = kAnimationUsedSeconds / kDurationSeconds;
	// 腕をアニメへ切り替える・戻す長さ（秒）。急に切り替えると腕が跳ねて見える。
	const float kArmBlendSeconds = 0.15f;
	// 挿す合図の前に、手の弾倉を銃の差し込み口へ寄せる長さ（秒）。手の位置と差し込み口はアニメで少しずれているので、跳ばずにはめるため。
	const float kMagazineSeatSeconds = 0.2f;

	// ── 音の合図（アニメの時刻。秒）。Blender で左手の動きを見て決めた ──
	const float kMagOutAnimationSeconds = 0.45f; // 左手が弾倉をつかんで下へ抜く
	const float kMagInAnimationSeconds = 1.6f;   // 左手が下から上がりきる（新しい弾倉を挿す）
	const float kBoltAnimationSeconds = 2.1f;    // 左手が右手へ一番近づく（銃の右の槓桿を引く）

	// アニメの時刻を、リロードを始めてからのティックに直す（1.25 倍で流すので早く来る）。
	int animationSecondsToTicks( float animationSeconds ) {
		return GameTime::secondsToTicks( animationSeconds / kAnimationRate );
	}
	const int kDurationTicks = GameTime::secondsToTicks( kDurationSeconds );
	const int kMagOutTicks = animationSecondsToTicks( kMagOutAnimationSeconds );
	const int kMagInTicks = animationSecondsToTicks( kMagInAnimationSeconds );
	const int kBoltTicks = animationSecondsToTicks( kBoltAnimationSeconds );
	const int kInactive = -1;

	// seconds が from → to の間をどれだけ進んだか（0〜1。曲線を掛けた値）。
	// 曲線は始めと終わりがゆっくりになる物（smoothstep）。直線だと腕が急に動き出し・急に止まって見えるため。
	float easedProgress( float seconds, float from, float to ) {
		float t = ( seconds - from ) / ( to - from );
		return ScalarMath::smoothStep( Math::min( 1.f, Math::max( 0.f, t ) ) );
	}
}

WeaponReload::WeaponReload() :
	mTicks( kInactive ),
	mCue( CUE_NONE ) {
}

void WeaponReload::start() {
	if ( !isActive() ) {
		mTicks = 0;
	}
}

void WeaponReload::cancel() {
	mTicks = kInactive;
}

bool WeaponReload::update() {
	mCue = CUE_NONE;
	if ( !isActive() ) {
		return false;
	}
	++mTicks;
	if ( mTicks == kMagOutTicks ) {
		mCue = CUE_MAG_OUT;
	} else if ( mTicks == kMagInTicks ) {
		mCue = CUE_MAG_IN;
	} else if ( mTicks == kBoltTicks ) {
		mCue = CUE_BOLT;
	}
	if ( mTicks < kDurationTicks ) {
		return false;
	}
	mTicks = kInactive;
	return true;
}

bool WeaponReload::isActive() const {
	return mTicks != kInactive;
}

WeaponReload::Cue WeaponReload::cueThisTick() const {
	return mCue;
}

float WeaponReload::animationFrame() const {
	if ( !isActive() ) {
		return 0.f;
	}
	return elapsedSeconds() * kAnimationRate * GameTime::kTicksPerSecond;
}

float WeaponReload::armWeight() const {
	if ( !isActive() ) {
		return 0.f;
	}
	float seconds = elapsedSeconds();
	return easedProgress( seconds, 0.f, kArmBlendSeconds ) * ( 1.f - easedProgress( seconds, kDurationSeconds - kArmBlendSeconds, kDurationSeconds ) );
}

bool WeaponReload::isMagazineOut() const {
	return isActive() && mTicks >= kMagOutTicks && mTicks < kMagInTicks;
}

float WeaponReload::magazineSeatWeight() const {
	if ( !isActive() ) {
		return 0.f;
	}
	float magInSeconds = GameTime::ticksToSeconds( kMagInTicks );
	return easedProgress( elapsedSeconds(), magInSeconds - kMagazineSeatSeconds, magInSeconds );
}

float WeaponReload::elapsedSeconds() const {
	return GameTime::ticksToSeconds( mTicks );
}
