#include "GameSrc/Weapon/WeaponReload.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// アニメ・合図の時刻・長さは銃ごとの表（WeaponCatalog の WeaponReloadSpec）。アニメは player.json の game_fps でゲームのフレームに直してある
	// 腕をアニメへ切り替える・戻す長さ（秒）。急に切り替えると腕が跳ねて見える
	const float kArmBlendSeconds = 0.15f;
	const float kMagazineSeatSeconds = 0.2f;	// 挿す合図の前に、手の弾倉を銃の差し込み口へ寄せる長さ（秒）。手の位置と差し込み口はアニメで少しずれているので、跳ばずにはめるため

	const int kInactive = -1;
	const int kNoTick = -1; // 来ない合図（スライドを引かないリロードの CUE_BOLT）

	// seconds が from → to の間をどれだけ進んだか（0〜1。曲線を掛けた値）
	// 曲線は始めと終わりがゆっくりになる物（smoothstep）。直線だと腕が急に動き出し・急に止まって見えるため
	float easedProgress( float seconds, float from, float to ) {
		float t = ( seconds - from ) / ( to - from );
		return ScalarMath::smoothStep( Math::min( 1.f, Math::max( 0.f, t ) ) );
	}
}

WeaponReload::WeaponReload( const WeaponReloadSpec& normal, const WeaponReloadSpec& empty ) :
	mNormal( &normal ),
	mEmpty( &empty ),
	mSpec( &normal ),
	mDurationTicks( 0 ),
	mMagOutTicks( 0 ),
	mMagInTicks( 0 ),
	mBoltTicks( kNoTick ),
	mTicks( kInactive ),
	mCue( CUE_NONE ) {
	ASSERT( normal.durationSeconds > 0.f && empty.durationSeconds > 0.f && "WeaponReload : durationSeconds must be positive." );
	select( false );
}

void WeaponReload::select( bool magazineEmpty ) {
	mSpec = magazineEmpty ? mEmpty : mNormal;
	mDurationTicks = GameTime::secondsToTicks( mSpec->durationSeconds );
	mMagOutTicks = animationSecondsToTicks( mSpec->magOutSeconds );
	mMagInTicks = animationSecondsToTicks( mSpec->magInSeconds );
	mBoltTicks = ( mSpec->boltSeconds < 0.f ) ? kNoTick : animationSecondsToTicks( mSpec->boltSeconds );
}

float WeaponReload::animationRate() const {
	return mSpec->animationSeconds / mSpec->durationSeconds;
}

const char* WeaponReload::animationName() const {
	return mSpec->animation;
}

int WeaponReload::animationSecondsToTicks( float animationSeconds ) const {
	return GameTime::secondsToTicks( animationSeconds / animationRate() );	// 速めて流すので、アニメの時刻より早く来る
}

void WeaponReload::start( bool magazineEmpty ) {
	if ( !isActive() ) {
		select( magazineEmpty );
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
	if ( mTicks == mMagOutTicks ) {
		mCue = CUE_MAG_OUT;
	} else if ( mTicks == mMagInTicks ) {
		mCue = CUE_MAG_IN;
	} else if ( mTicks == mBoltTicks ) {
		mCue = CUE_BOLT;
	}
	if ( mTicks < mDurationTicks ) {
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
	return elapsedSeconds() * animationRate() * GameTime::kTicksPerSecond;
}

float WeaponReload::armWeight() const {
	if ( !isActive() ) {
		return 0.f;
	}
	float seconds = elapsedSeconds();
	return easedProgress( seconds, 0.f, kArmBlendSeconds ) * ( 1.f - easedProgress( seconds, mSpec->durationSeconds - kArmBlendSeconds, mSpec->durationSeconds ) );
}

bool WeaponReload::isMagazineOut() const {
	return isActive() && mTicks >= mMagOutTicks && mTicks < mMagInTicks;
}

float WeaponReload::magazineSeatWeight() const {
	if ( !isActive() ) {
		return 0.f;
	}
	float magInSeconds = GameTime::ticksToSeconds( mMagInTicks );
	return easedProgress( elapsedSeconds(), magInSeconds - kMagazineSeatSeconds, magInSeconds );
}

float WeaponReload::elapsedSeconds() const {
	return GameTime::ticksToSeconds( mTicks );
}
