#include "GameSrc/Player/Camera/AimSway.h"
#include "GameSrc/Player/Camera/AimSwayParams.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Math/ScalarMath.h"
#include "GameSrc/Math/AngleMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	AimSwayParams gParams;	// 大きさ・速さは AimSwayParams（起動中に F6 で動かせる）

	const float kStepsPerCycle = 2.f;	// 歩きのアニメ 1 周の歩数（上下は 1 歩ごとに 1 回沈む）
	const float kHalf = 0.5f;
	const float kMinLag = 0.001f;		// これより小さくなった遅れは 0 にする（いつまでも小さな値で狙いを動かし続けないため）

	// from から to への近い方の差（-0.5〜0.5）。0.95 → 0.05 なら +0.1（逆向きに 0.9 戻らない）
	float phaseGap( float from, float to ) {
		return ScalarMath::wrapUnit( to - from + kHalf ) - kHalf;
	}

	// seconds 秒で 0 → 1 進むときの、1 ティックあたりの量
	float perTick( float seconds ) {
		return 1.f / ( seconds * GameTime::kTicksPerSecond );
	}

	// 体力の状態に合わせた倍率
	float conditionScale( bool danger ) {
		return danger ? gParams.dangerScale : 1.f;
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
	mBreathYawPhase = ScalarMath::wrapUnit( mBreathYawPhase + perTick( gParams.breathYawPeriod ) );
	mBreathPitchPhase = ScalarMath::wrapUnit( mBreathPitchPhase + perTick( gParams.breathPitchPeriod ) );
	float yaw = gParams.breathYaw * Math::sin( AngleMath::kFullTurnDegree * mBreathYawPhase );
	float pitch = gParams.breathPitch * Math::sin( AngleMath::kFullTurnDegree * mBreathPitchPhase );

	// b. 歩き：強さは速さの割合へ、上がるときは速く、下がるときはゆっくり寄せる
	float target = ScalarMath::clamp( body.walkRate, 0.f, 1.f );
	if ( mWalkStrength < target ) {
		mWalkStrength = Math::min( target, mWalkStrength + perTick( gParams.walkRiseSeconds ) );
	} else {
		mWalkStrength = Math::max( target, mWalkStrength - perTick( gParams.walkFallSeconds ) );
	}
	// 位相は速さの割合で自分で進め（止まれば止まる）、歩き・走りのアニメ中だけその位相へ少しずつ寄せる
	// アニメの位相をそのまま使うと、A・D を素早く押し替えたときに止まる⇔歩くでアニメが頭から始まり、銃が 1 ティックで跳ぶ
	mWalkPhase = ScalarMath::wrapUnit( mWalkPhase + target * perTick( gParams.walkCycleSeconds ) );
	if ( body.gaitCycling ) {
		float pull = 1.f - GameTime::remainPerTick( gParams.walkSyncHalfLife );
		mWalkPhase = ScalarMath::wrapUnit( mWalkPhase + phaseGap( mWalkPhase, body.gaitPhase ) * pull );
	}
	// 上下は 1 歩ごとに 1 往復、左右は 1 周（2 歩）で 1 往復（足を着くたびに沈み、体重が左右へ移るのに合わせて振れる）
	yaw += gParams.walkYaw * mWalkStrength * Math::sin( AngleMath::kFullTurnDegree * mWalkPhase );
	pitch += gParams.walkPitch * mWalkStrength * Math::sin( AngleMath::kFullTurnDegree * kStepsPerCycle * mWalkPhase );

	// c. 振り向きの遅れ：回した向きと逆へ残し、ばねで 0 へ戻す
	float remain = GameTime::remainPerTick( gParams.lagHalfLife );
	mLagYaw = ScalarMath::clampSigned( ScalarMath::decay( mLagYaw, remain, kMinLag ) - lookYaw * gParams.lagRate, gParams.lagLimit );
	mLagPitch = ScalarMath::clampSigned( ScalarMath::decay( mLagPitch, remain, kMinLag ) - lookPitch * gParams.lagRate, gParams.lagLimit );
	yaw += mLagYaw;
	pitch += mLagPitch;

	// 腰だめは大きく（覗き具合で 1 倍との間を取る）、腕が壊れていれば大きく、抱えている間・素手の間は 0
	float scale = ScalarMath::lerp( gParams.hipScale, 1.f, body.aimWeight );
	scale *= conditionScale( body.danger ) * ( 1.f - body.carryWeight );
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
