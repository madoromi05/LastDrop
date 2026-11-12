#include "GameSrc/Weapon/Recoil/WeaponRecoil.h"
#include "GameSrc/Math/ScalarMath.h"

namespace{
	const float kFullKick = 1.f;
	// 1ティックごとに残る割合。連射の間隔（6ティック）の間に約 1/64 まで戻るので、
	// 連射でも1発ごとに「跳ねる → 戻る」がはっきり見える（視点の ViewRecoil と同じ速さに揃えてある）
	// 大きくすると戻りが遅くなり、連射で銃が引かれたままに見える
	const float kRemainPerTick = 0.5f;
	const float kMinKick = 0.001f;	// これより小さくなったら 0 にする（いつまでも小さな値で銃を動かし続けないため）
}

WeaponRecoil::WeaponRecoil() :
	mKick( 0.f ) {
}

void WeaponRecoil::update( bool fired ) {
	if ( fired ) {
		// 戻りきる前に撃っても足さない（連射で反動が積み上がって銃が画面の外へ出ないように）
		mKick = kFullKick;
		return;
	}
	mKick = ScalarMath::decay( mKick, kRemainPerTick, kMinKick );
}

float WeaponRecoil::kick() const {
	return mKick;
}
