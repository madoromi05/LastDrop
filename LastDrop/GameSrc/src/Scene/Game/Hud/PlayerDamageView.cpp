#include "GameSrc/Scene/Game/Hud/PlayerDamageView.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// 攻撃された瞬間の赤みが消えるまで（秒）。攻撃された瞬間が一番赤く、まっすぐ薄くなる
	const float kFlashSeconds = 0.6f;
	// 体力が減ったときに残し続ける赤み（0〜1。攻撃された瞬間の 1 より薄く）
	// 残りがこの割合以下で赤くし始め、その時は Min、0 の手前で Max。間はまっすぐ濃くなる
	// 攻撃された赤みが薄くなってこれを下回ると、ここで止まる
	const float kHurtStartRatio = 0.5f;
	const float kHurtFlashMin = 0.15f;
	const float kHurtFlashMax = 0.6f;
	const float kNoFlash = 0.f;
	// 攻撃された瞬間の痛み（周りのぼかし・暗さ）が消えるまで（秒）。赤みとは別に直せるよう分けておく
	const float kPainSeconds = 0.6f;
	// 死んだ後、視点が下がり切るまで（秒）と下げる長さ（m。立った目の高さ約 1.6m から、膝をついた高さまで）
	const float kDropSeconds = 1.5f;
	const float kDropMeters = 1.f;
	// 死んだ後、真っ暗になるまで（秒）。視点が下がり切るより少し後に暗くなり終える
	const float kFadeSeconds = 2.f;

	const int kFlashTicks = GameTime::secondsToTicks( kFlashSeconds );
	const int kPainTicks = GameTime::secondsToTicks( kPainSeconds );
	const int kDropTicks = GameTime::secondsToTicks( kDropSeconds );
	const int kFadeTicks = GameTime::secondsToTicks( kFadeSeconds );

	// ticks が total のうち何割まで進んだか（0〜1）
	float progress( int ticks, int total ) {
		return Math::min( 1.f, static_cast< float >( ticks ) / static_cast< float >( total ) );
	}

	// 体力の残りの割合から、残し続ける赤み
	float hurtFlash( float healthRatio ) {
		if ( healthRatio > kHurtStartRatio ) {
			return kNoFlash;
		}
		// 赤くし始める割合で 0、残り 0 で 1
		float depth = 1.f - healthRatio / kHurtStartRatio;
		return ScalarMath::lerp( kHurtFlashMin, kHurtFlashMax, depth );
	}
}

PlayerDamageView::PlayerDamageView() :
	mFlashTicks( 0 ),
	mPainTicks( 0 ),
	mDeadTicks( 0 ),
	mHealthRatio( 1.f ),
	mDead( false ) {
}

void PlayerDamageView::update( bool attacked, float healthRatio, bool dead ) {
	mHealthRatio = healthRatio;
	if ( attacked ) {
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
	return Math::max( progress( mFlashTicks, kFlashTicks ), hurtFlash( mHealthRatio ) );
}

float PlayerDamageView::pain() const {
	return progress( mPainTicks, kPainTicks );
}

float PlayerDamageView::eyeDrop() const {
	// 始めと終わりがゆっくりになる曲線（smoothstep）にする。崩れ始め・崩れ終わりが急に見えないように
	return kDropMeters * ScalarMath::smoothStep( progress( mDeadTicks, kDropTicks ) );
}

float PlayerDamageView::fade() const {
	return progress( mDeadTicks, kFadeTicks );
}

bool PlayerDamageView::isFadedOut() const {
	return mDead && mDeadTicks >= kFadeTicks;
}
