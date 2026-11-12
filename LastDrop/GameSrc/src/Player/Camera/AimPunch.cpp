#include "GameSrc/Player/Camera/AimPunch.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// 1 回で跳ねる角度（度）。上へ kPunchPitch、左右へ −kPunchYaw〜+kPunchYaw
	const float kPunchPitch = 2.5f;
	const float kPunchYaw = 2.0f;
	// 1 ティックごとに残る割合。0.85 で約 0.3 秒（18 ティックで 1/19）まで戻る
	const float kReturnPerTick = 0.85f;
	// 続けて攻撃されても跳ねすぎないように、合計はこれで止める（度）
	const float kLimit = 6.f;
	// これより小さくなったら 0 にする
	const float kMinOffset = 0.001f;

	float settle( float value ) {
		value *= kReturnPerTick;
		return ( Math::abs( value ) < kMinOffset ) ? 0.f : value;
	}
}

AimPunch::AimPunch( int seed ) :
	mRandom( Math::Random::create( seed ) ),
	mRise( 0.f ),
	mSide( 0.f ) {
}

void AimPunch::punch() {
	mRise = Math::min( kLimit, mRise + kPunchPitch );
	mSide = Math::max( -kLimit, Math::min( kLimit, mSide + mRandom.getFloat( -kPunchYaw, kPunchYaw ) ) );
}

void AimPunch::update() {
	mRise = settle( mRise );
	mSide = settle( mSide );
}

float AimPunch::pitchOffset() const {
	return -mRise; // ピッチは正で見下ろすので、上へ跳ねる分は負
}

float AimPunch::yawOffset() const {
	return mSide;
}
