#include "GameSrc/Weapon/WeaponStance.h"
#include "GameSrc/Weapon/WeaponStanceParams.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	WeaponStanceParams gParams;	// 短押しの長さ・覗くまでの時間は WeaponStanceParams（秒。起動中に F6 で動かせる）。ティックへは使うたびに換算する

	// これより短く押して離したら「覗いたまま」にする（ティック）
	int tapTicks() {
		return GameTime::secondsToTicks( gParams.tapSeconds );
	}

	// 覗き具合が 1 ティックに進む量（覗き始めてからサイトが目の前に来るまでで 1）
	float aimStep() {
		return 1.f / ( gParams.aimSeconds * GameTime::kTicksPerSecond );
	}

	// 始めと終わりがゆっくりになる曲線（smoothstep）。直線だと銃が急に動き出し・急に止まって見えるため
	float easeInOut( float t ) {
		return t * t * ( 3.f - 2.f * t );
	}
}

WeaponStanceParams& weaponStanceParams() {
	return gParams;
}

WeaponStance::WeaponStance() :
	mAimMode( AIM_OFF ),
	mAimWasHeld( false ),
	mRunWasHeld( false ),
	mAimHeldFrames( 0 ),
	mAimProgress( 0.f ) {
}

void WeaponStance::update( bool aimHeld, bool runHeld ) {
	updateAimMode( aimHeld );
	// 走り出そうとした瞬間だけ、覗いたままをやめる。押し続けている走る入力でやめないのは、
	// 走りながら覗いた（＝歩きに落とした）直後に、押したままの走る入力で覗きが消えないようにするため
	// 長押しで覗いている間は、覗く入力を押している方を優先して覗き続ける
	bool runPressed = runHeld && !mRunWasHeld;
	mRunWasHeld = runHeld;
	if ( runPressed && mAimMode == AIM_TOGGLE ) {
		mAimMode = AIM_OFF;
	}

	float target = isAiming() ? 1.f : 0.f;
	if ( mAimProgress < target ) {
		mAimProgress = Math::min( target, mAimProgress + aimStep() );
	} else {
		mAimProgress = Math::max( target, mAimProgress - aimStep() );
	}
}

void WeaponStance::updateAimMode( bool aimHeld ) {
	bool pressed = aimHeld && !mAimWasHeld;
	bool released = !aimHeld && mAimWasHeld;
	mAimWasHeld = aimHeld;

	switch ( mAimMode ) {
	case AIM_OFF:
		// 走っている間に押せば覗き始める（走りは呼び出し側が歩きに落とす）
		if ( pressed ) {
			mAimMode = AIM_HOLD;
			mAimHeldFrames = 0;
		}
		break;
	case AIM_HOLD:
		if ( released ) {
			mAimMode = ( mAimHeldFrames < tapTicks() ) ? AIM_TOGGLE : AIM_OFF;
		} else {
			++mAimHeldFrames;
		}
		break;
	case AIM_TOGGLE:
		if ( pressed ) {
			mAimMode = AIM_OFF;
		}
		break;
	}
}

void WeaponStance::stopAiming() {
	mAimMode = AIM_OFF;
}

bool WeaponStance::isAiming() const {
	return mAimMode != AIM_OFF;
}

float WeaponStance::aimWeight() const {
	return easeInOut( mAimProgress );
}
