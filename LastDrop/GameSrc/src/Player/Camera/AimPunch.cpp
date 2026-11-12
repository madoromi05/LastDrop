#include "GameSrc/Player/Camera/AimPunch.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	// 1 回で跳ねる角度（度）。上へ kPunchPitch、左右へ −kPunchYaw〜+kPunchYaw
	const float kPunchPitch = 2.5f;
	const float kPunchYaw = 2.0f;
	const float kReturnPerTick = 0.85f;	// 1 ティックごとに残る割合。0.85 で約 0.3 秒（18 ティックで 1/19）まで戻る
	const float kLimit = 6.f;			// 続けて攻撃されても跳ねすぎないように、合計はこれで止める（度）
	const float kMinOffset = 0.001f;	// これより小さくなったら 0 にする
}

AimPunch::AimPunch( int seed ) :
	mRandom( Math::Random::create( seed ) ),
	mRise( 0.f ),
	mSide( 0.f ) {
}

void AimPunch::punch() {
	mRise = Math::min( kLimit, mRise + kPunchPitch );
	mSide = ScalarMath::clampSigned( mSide + mRandom.getFloat( -kPunchYaw, kPunchYaw ), kLimit );
}

void AimPunch::update() {
	mRise = ScalarMath::decay( mRise, kReturnPerTick, kMinOffset );
	mSide = ScalarMath::decay( mSide, kReturnPerTick, kMinOffset );
}

float AimPunch::pitchOffset() const {
	return -mRise; // ピッチは正で見下ろすので、上へ跳ねる分は負
}

float AimPunch::yawOffset() const {
	return mSide;
}
