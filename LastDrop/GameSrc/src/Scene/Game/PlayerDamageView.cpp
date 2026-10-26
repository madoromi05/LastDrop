#include "GameSrc/Scene/Game/PlayerDamageView.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// 噛まれた瞬間の赤みが消えるまで（秒）。噛まれた瞬間が一番赤く、まっすぐ薄くなる。
	const float kFlashSeconds = 0.6f;
	// 噛まれた瞬間の痛み（周りのぼかし・暗さ）が消えるまで（秒）。赤みとは別に直せるよう分けておく。
	const float kPainSeconds = 0.6f;
	// 死んだ後、視点が下がり切るまで（秒）と下げる長さ（m。立った目の高さ約 1.6m から、膝をついた高さまで）。
	const float kDropSeconds = 1.5f;
	const float kDropMeters = 1.f;
	// 死んだ後、真っ暗になるまで（秒）。視点が下がり切るより少し後に暗くなり終える。
	const float kFadeSeconds = 2.f;

	const int kFlashTicks = GameTime::secondsToTicks( kFlashSeconds );
	const int kPainTicks = GameTime::secondsToTicks( kPainSeconds );
	const int kDropTicks = GameTime::secondsToTicks( kDropSeconds );
	const int kFadeTicks = GameTime::secondsToTicks( kFadeSeconds );

	// ticks が total のうち何割まで進んだか（0〜1）。
	float progress( int ticks, int total ) {
		return Math::min( 1.f, static_cast< float >( ticks ) / static_cast< float >( total ) );
	}
}

PlayerDamageView::PlayerDamageView() :
	mFlashTicks( 0 ),
	mPainTicks( 0 ),
	mDeadTicks( 0 ),
	mDead( false ) {
}

void PlayerDamageView::update( bool bitten, bool dead ) {
	if ( bitten ) {
		mFlashTicks = kFlashTicks;
		mPainTicks = kPainTicks;
	} else {
		if ( mFlashTicks > 0 ) {
			--mFlashTicks;
		}
		if ( mPainTicks > 0 ) {
			--mPainTicks;
		}
	}
	mDead = dead;
	if ( mDead && mDeadTicks < kFadeTicks ) {
		++mDeadTicks;
	}
}

float PlayerDamageView::damageFlash() const {
	return progress( mFlashTicks, kFlashTicks );
}

float PlayerDamageView::pain() const {
	return progress( mPainTicks, kPainTicks );
}

float PlayerDamageView::eyeDrop() const {
	// 始めと終わりがゆっくりになる曲線（smoothstep）にする。崩れ始め・崩れ終わりが急に見えないように。
	return kDropMeters * ScalarMath::smoothStep( progress( mDeadTicks, kDropTicks ) );
}

float PlayerDamageView::fade() const {
	return progress( mDeadTicks, kFadeTicks );
}

bool PlayerDamageView::isFadedOut() const {
	return mDead && mDeadTicks >= kFadeTicks;
}
