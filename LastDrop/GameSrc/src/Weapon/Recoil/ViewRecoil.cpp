#include "GameSrc/Weapon/Recoil/ViewRecoil.h"

namespace{
	const float kRisePerShot = 1.30f;	// 1発で視点が跳ね上がる角度（度。ライフル〈反動の大きさ 1〉のとき）。「少しだけ」の指定なので、狙いが大きく外れない小ささにする
	// 1ティックごとに残る割合。連射の間隔（6ティック）の間に約 1/64 まで戻るので、
	// 連射でも1発ごとに「跳ねる → 戻る」がはっきり見える（上がったままにしない）
	// 大きくすると戻りが遅くなり、次の1発までに戻りきらなくなる
	const float kRemainPerTick = 0.5f;
	const float kMinRise = 0.001f;		// これより小さくなったら 0 にする（いつまでも小さな値で視点を動かし続けないため）
}

ViewRecoil::ViewRecoil() :
	mRise( 0.f ) {
}

void ViewRecoil::update( float shotRecoil ) {
	if ( shotRecoil > 0.f ) {
		// 戻りきる前に撃っても足さない（連射で視点が積み上がって上がり続けないように。毎発同じ高さまで跳ねる）
		mRise = kRisePerShot * shotRecoil;
		return;
	}
	mRise *= kRemainPerTick;
	if ( mRise < kMinRise ) {
		mRise = 0.f;
	}
}

float ViewRecoil::pitchOffset() const {
	return -mRise; // ピッチは正で見下ろすので、跳ね上がりは負
}
