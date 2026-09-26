#include "WeaponStance.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	const float kFramesPerSecond = 60.f;
	// これより短く押して離したら「覗いたまま」にする（フレーム）。0.2秒。Tarkov の短押し判定に合わせた体感値。
	const int kTapFrames = static_cast< int >( 0.2f * kFramesPerSecond );
	// 覗き始めてからサイトが目の前に来るまで（フレーム）。0.25秒。短すぎると銃が瞬間移動して見え、長いと操作が重い。
	const float kAimFrames = 0.25f * kFramesPerSecond;
	const float kAimStep = 1.f / kAimFrames;

	// 始めと終わりがゆっくりになる曲線（smoothstep）。直線だと銃が急に動き出し・急に止まって見えるため。
	float easeInOut( float t ) {
		return t * t * ( 3.f - 2.f * t );
	}
}

WeaponStance::WeaponStance() :
	mDrawn( true ), // 始めから武器を持って出る（Tarkov と同じ）
	mAimMode( AIM_OFF ),
	mAimWasHeld( false ),
	mAimHeldFrames( 0 ),
	mAimProgress( 0.f ) {
}

void WeaponStance::update( bool holsterPressed, bool aimHeld, bool running ) {
	if ( holsterPressed ) {
		mDrawn = !mDrawn;
	}
	updateAimMode( aimHeld );
	if ( !mDrawn || running ) {
		mAimMode = AIM_OFF;
	}

	float target = isAiming() ? 1.f : 0.f;
	if ( mAimProgress < target ) {
		mAimProgress = Math::min( target, mAimProgress + kAimStep );
	} else {
		mAimProgress = Math::max( target, mAimProgress - kAimStep );
	}
}

void WeaponStance::updateAimMode( bool aimHeld ) {
	bool pressed = aimHeld && !mAimWasHeld;
	bool released = !aimHeld && mAimWasHeld;
	mAimWasHeld = aimHeld;

	switch ( mAimMode ) {
	case AIM_OFF:
		// しまっている・走っている間に押しても、update が直後に AIM_OFF へ戻す。
		// 押した瞬間でしか覗き始めないので、押したまま走りをやめても勝手に覗かない。
		if ( pressed ) {
			mAimMode = AIM_HOLD;
			mAimHeldFrames = 0;
		}
		break;
	case AIM_HOLD:
		if ( released ) {
			mAimMode = ( mAimHeldFrames < kTapFrames ) ? AIM_TOGGLE : AIM_OFF;
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

bool WeaponStance::isDrawn() const {
	return mDrawn;
}

bool WeaponStance::isAiming() const {
	return mDrawn && mAimMode != AIM_OFF;
}

float WeaponStance::aimWeight() const {
	return easeInOut( mAimProgress );
}
