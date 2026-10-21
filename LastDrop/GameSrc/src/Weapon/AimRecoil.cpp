#include "GameSrc/Weapon/AimRecoil.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// ── 上（度）──
	// 1 発目に上がる角度。2 発目からはだんだん小さくし、kClimbShots 発目で 0 にする（数発で頭打ち。ユーザー指定）。
	// 合計は kRiseFirst × (kClimbShots + 1) / 2 ＝ 3.5°。
	const float kRiseFirst = 1.0f;
	const int kClimbShots = 6;
	// 頭打ちの後は、1 発ごとに上下へ ±kRiseBounce を足し、頭打ちの高さ ±kRiseBounceLimit に収める（上がり続けず、小さく揺れるだけ）。
	const float kRiseBounce = 0.3f;
	const float kRiseBounceLimit = 0.6f;
	const float kRiseTop = kRiseFirst * static_cast< float >( kClimbShots + 1 ) * 0.5f;

	// ── 左右（度）──
	// 1 発ごとに −kSidePerShot〜+kSidePerShot を一様乱数で足し、合計は ±kSideLimit で止める。
	const float kSidePerShot = 0.45f;
	const float kSideLimit = 2.5f;

	// ── 戻り ──
	// 最後の 1 発からこれだけ撃たなければ「撃ちやめた」とする。連射の間隔（FireControl の 600 発/分＝6 ティック）より 1 長く、
	// 連射の途中で戻り始めないようにする。
	const int kSettleTicks = 7;
	// 撃ちやめた後、1 ティックごとに残る割合。0.9 で約 0.4 秒（24 ティックで 1/12）まで戻る。
	const float kReturnPerTick = 0.9f;
	// これより小さくなったら 0 にする（いつまでも小さな値で狙いを動かし続けないため）。
	const float kMinOffset = 0.001f;

	float clampSigned( float value, float limit ) {
		return Math::max( -limit, Math::min( limit, value ) );
	}

	float settle( float value ) {
		value *= kReturnPerTick;
		return ( Math::abs( value ) < kMinOffset ) ? 0.f : value;
	}
}

AimRecoil::AimRecoil( int seed ) :
	mRandom( Math::Random::create( seed ) ),
	mRise( 0.f ),
	mSide( 0.f ),
	mShotIndex( 0 ),
	mQuietTicks( kSettleTicks ) {
}

void AimRecoil::update( bool fired ) {
	if ( fired ) {
		kick();
		mQuietTicks = 0;
		return;
	}
	if ( mQuietTicks < kSettleTicks ) {
		++mQuietTicks;
		return; // 連射の間。ずれはたまったまま
	}
	mShotIndex = 0;
	mRise = settle( mRise );
	mSide = settle( mSide );
}

void AimRecoil::kick() {
	++mShotIndex;
	if ( mShotIndex <= kClimbShots ) {
		float share = 1.f - static_cast< float >( mShotIndex - 1 ) / static_cast< float >( kClimbShots );
		mRise += kRiseFirst * share;
	} else {
		mRise += mRandom.getFloat( -kRiseBounce, kRiseBounce );
		mRise = kRiseTop + clampSigned( mRise - kRiseTop, kRiseBounceLimit );
	}
	mSide = clampSigned( mSide + mRandom.getFloat( -kSidePerShot, kSidePerShot ), kSideLimit );
}

float AimRecoil::pitchOffset() const {
	return -mRise; // ピッチは正で見下ろすので、上がる分は負
}

float AimRecoil::yawOffset() const {
	return mSide;
}
