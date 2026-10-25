#include "GameSrc/Player/AimSway.h"
#include "GameSrc/Player/AimSwayParams.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// 大きさ・速さは AimSwayParams（起動中に F6 で動かせる）。
	AimSwayParams gParams;

	const float kFullTurn = 360.f;     // 波の 1 往復（度。GameLib の sin は度で受ける）
	const float kStepsPerCycle = 2.f;  // 歩きのアニメ 1 周の歩数（上下は 1 歩ごとに 1 回沈む）
	const float kHalf = 0.5f;
	// これより遅ければ「歩いていない」とみなし、歩きの位相を止める（止まった後も形がアニメの位相で動き続けないように）。
	const float kMovingRate = 0.05f;
	// これより小さくなった遅れは 0 にする（いつまでも小さな値で狙いを動かし続けないため）。
	const float kMinLag = 0.001f;
	const int kOneArm = 1;
	const int kTwoArms = 2;

	// 進み具合 phase（0〜1）を step だけ進め、1 を超えたら折り返す。
	float advance( float phase, float step ) {
		phase += step;
		return ( phase >= 1.f ) ? phase - 1.f : phase;
	}

	// seconds 秒で 0 → 1 進むときの、1 ティックあたりの量。
	float perTick( float seconds ) {
		return 1.f / ( seconds * GameTime::kTicksPerSecond );
	}

	float clampSigned( float value, float limit ) {
		return Math::max( -limit, Math::min( limit, value ) );
	}

	// 半分に戻るまで halfLife 秒のとき、1 ティックごとに残る割合（はね返らない減り方）。
	float remainPerTick( float halfLife ) {
		return Math::pow( kHalf, perTick( halfLife ) );
	}

	float settle( float value, float remain ) {
		value *= remain;
		return ( Math::abs( value ) < kMinLag ) ? 0.f : value;
	}

	// 腕の壊れ方に合わせた倍率。
	float armScale( int brokenArms ) {
		if ( brokenArms >= kTwoArms ) {
			return gParams.twoArmScale;
		}
		return ( brokenArms == kOneArm ) ? gParams.oneArmScale : 1.f;
	}
}

AimSwayParams& aimSwayParams() {
	return gParams;
}

AimSway::AimSway() :
	mBreathYawPhase( 0.f ),
	mBreathPitchPhase( 0.f ),
	mWalkStrength( 0.f ),
	mWalkPhase( 0.f ),
	mLagYaw( 0.f ),
	mLagPitch( 0.f ),
	mYaw( 0.f ),
	mPitch( 0.f ) {
}

void AimSway::update( const AimSwayBody& body, float lookYaw, float lookPitch ) {
	// a. 息
	mBreathYawPhase = advance( mBreathYawPhase, perTick( gParams.breathYawPeriod ) );
	mBreathPitchPhase = advance( mBreathPitchPhase, perTick( gParams.breathPitchPeriod ) );
	float yaw = gParams.breathYaw * Math::sin( kFullTurn * mBreathYawPhase );
	float pitch = gParams.breathPitch * Math::sin( kFullTurn * mBreathPitchPhase );

	// b. 歩き：強さは速さの割合へ、上がるときは速く、下がるときはゆっくり寄せる。
	float target = Math::max( 0.f, Math::min( 1.f, body.walkRate ) );
	if ( mWalkStrength < target ) {
		mWalkStrength = Math::min( target, mWalkStrength + perTick( gParams.walkRiseSeconds ) );
	} else {
		mWalkStrength = Math::max( target, mWalkStrength - perTick( gParams.walkFallSeconds ) );
	}
	if ( body.walkRate > kMovingRate ) {
		mWalkPhase = body.gaitPhase;
	}
	// 上下は 1 歩ごとに 1 往復、左右は 1 周（2 歩）で 1 往復（足を着くたびに沈み、体重が左右へ移るのに合わせて振れる）。
	yaw += gParams.walkYaw * mWalkStrength * Math::sin( kFullTurn * mWalkPhase );
	pitch += gParams.walkPitch * mWalkStrength * Math::sin( kFullTurn * kStepsPerCycle * mWalkPhase );

	// c. 振り向きの遅れ：回した向きと逆へ残し、ばねで 0 へ戻す。
	float remain = remainPerTick( gParams.lagHalfLife );
	mLagYaw = clampSigned( settle( mLagYaw, remain ) - lookYaw * gParams.lagRate, gParams.lagLimit );
	mLagPitch = clampSigned( settle( mLagPitch, remain ) - lookPitch * gParams.lagRate, gParams.lagLimit );
	yaw += mLagYaw;
	pitch += mLagPitch;

	// 腰だめは大きく（覗き具合で 1 倍との間を取る）、腕が壊れていれば大きく、抱えている間・しまっている間は 0。
	float scale = gParams.hipScale + ( 1.f - gParams.hipScale ) * body.aimWeight;
	scale *= armScale( body.brokenArms ) * ( 1.f - body.carryWeight );
	if ( !body.weaponDrawn ) {
		scale = 0.f;
	}
	mYaw = yaw * scale;
	mPitch = pitch * scale;
}

float AimSway::yawOffset() const {
	return mYaw;
}

float AimSway::pitchOffset() const {
	return mPitch;
}
