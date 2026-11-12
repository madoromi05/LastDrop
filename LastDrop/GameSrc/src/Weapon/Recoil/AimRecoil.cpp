#include "GameSrc/Weapon/Recoil/AimRecoil.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// ── 上（度。ライフル〈反動の大きさ 1〉のとき。ほかの銃は反動の大きさを掛ける） ──
	// 1 発目に上がる角度。2 発目からはだんだん小さくし、kClimbShots 発目で 0 にする（数発で頭打ち）
	// 合計は kRiseFirst × (kClimbShots + 1) / 2 ＝ 3.5°
	const float kRiseFirst = 1.0f;
	const int kClimbShots = 6;
	// 頭打ちの後は、1 発ごとに上下へ ±kRiseBounce を足し、頭打ちの高さ ±kRiseBounceLimit に収める（上がり続けず、小さく揺れるだけ）
	const float kRiseBounce = 0.3f;
	const float kRiseBounceLimit = 0.6f;
	const float kRiseTop = kRiseFirst * static_cast< float >( kClimbShots + 1 ) * 0.5f;

	// ── 左右（度）──
	// 1 発ごとに −kSidePerShot〜+kSidePerShot を一様乱数で足し、合計は ±kSideLimit で止める
	const float kSidePerShot = 0.45f;
	const float kSideLimit = 2.5f;

	// ── 戻り ──
	// 最後の 1 発からこれだけ撃たなければ「撃ちやめた」とする。連射の間隔（FireControl の 600 発/分＝6 ティック）より 1 長く、
	// 連射の途中で戻り始めないようにする
	const int kSettleTicks = 7;
	const float kReturnPerTick = 0.9f;	// 撃ちやめた後、1 ティックごとに残る割合。0.9 で約 0.4 秒（24 ティックで 1/12）まで戻る
	const float kMinOffset = 0.001f;	// これより小さくなったら 0 にする（いつまでも小さな値で狙いを動かし続けないため）

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

void AimRecoil::update( float shotRecoil ) {
	if ( shotRecoil > 0.f ) {
		kick( shotRecoil );
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

void AimRecoil::kick( float scale ) {
	++mShotIndex;
	if ( mShotIndex <= kClimbShots ) {
		float share = 1.f - static_cast< float >( mShotIndex - 1 ) / static_cast< float >( kClimbShots );
		mRise += kRiseFirst * share * scale;
	} else {
		float top = kRiseTop * scale; // 頭打ちの高さも反動の大きさに合わせる
		mRise += mRandom.getFloat( -kRiseBounce, kRiseBounce ) * scale;
		mRise = top + ScalarMath::clampSigned( mRise - top, kRiseBounceLimit * scale );
	}
	mSide = ScalarMath::clampSigned( mSide + mRandom.getFloat( -kSidePerShot, kSidePerShot ) * scale, kSideLimit );
}

float AimRecoil::pitchOffset() const {
	return -mRise; // ピッチは正で見下ろすので、上がる分は負
}

float AimRecoil::yawOffset() const {
	return mSide;
}
