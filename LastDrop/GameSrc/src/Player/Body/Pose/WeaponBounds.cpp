#include "GameSrc/Player/Body/Pose/WeaponBounds.h"
#include "GameSrc/Player/Body/Pose/FirstPersonRig.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

WeaponBounds::WeaponBounds( const FirstPersonRig* rig ) :
	mRig( rig ),
	mHasBounds( false ),
	mLow( 0.f, 0.f, 0.f ),
	mHigh( 0.f, 0.f, 0.f ) {
	ASSERT( mRig && "WeaponBounds : rig is null." );
}

void WeaponBounds::reset() {
	mHasBounds = false;
}

bool WeaponBounds::bounds( Vector3* low, Vector3* high ) const {
	if ( !mHasBounds ) {
		return false;
	}
	*low = mLow;
	*high = mHigh;
	return true;
}

void WeaponBounds::apply( std::vector< Matrix34 >* pose ) {
	if ( !mRig->hasWeapon() ) {
		return;
	}
	// 今のポーズでの銃の置き場所（HeldItem が描くのと同じ「手の骨 * 持ち方」）で、頂点の範囲の8隅を移して囲む
	Matrix34 gun = mRig->weaponPlacement( *pose );
	const int kCornerNumber = 8;
	for ( int i = 0; i < kCornerNumber; ++i ) {
		// i の各ビットで、その軸を最小・最大のどちらにするかを選ぶ（8通りで8隅）
		Vector3 corner(
			( i & 1 ) ? mRig->weaponMax.x : mRig->weaponMin.x,
			( i & 2 ) ? mRig->weaponMax.y : mRig->weaponMin.y,
			( i & 4 ) ? mRig->weaponMax.z : mRig->weaponMin.z );
		Vector3 placed;
		gun.mul( &placed, corner );
		if ( i == 0 ) {
			mLow = placed;
			mHigh = placed;
			continue;
		}
		mLow.set( Math::min( mLow.x, placed.x ), Math::min( mLow.y, placed.y ), Math::min( mLow.z, placed.z ) );
		mHigh.set( Math::max( mHigh.x, placed.x ), Math::max( mHigh.y, placed.y ), Math::max( mHigh.z, placed.z ) );
	}
	mHasBounds = true;
}
