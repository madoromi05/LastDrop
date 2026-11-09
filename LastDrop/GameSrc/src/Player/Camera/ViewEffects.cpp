#include "GameSrc/Player/Camera/ViewEffects.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "Framework.h"

#include "GameLib/GameLib.h"

using namespace GameLib;

namespace{
	// 見える絵の揺れ（CameraShake のトラウマ。0〜1）。仕様は SummaryPlan/個別仕様/射撃と手応え/CameraShake_Recoil_Plan.txt の 3・4 章。
	// 射撃は 1 発ごとに足すが、射撃だけでは kFireTraumaLimit より揺らさない（連射しても揺れすぎない）。
	const float kFireTrauma = 0.12f;
	const float kFireTraumaLimit = 0.45f;
	const float kZombieHitTrauma = 0.8f;
	const float kGrabTrauma = 0.35f; // つかまれた瞬間（噛まれたときより弱く）
	// 撃った手応えの揺れ（HitFeel_Plan.txt Q3）。部位が飛んだ＝小さく・頭が弾けた＝少し大きく。
	// 噛まれた揺れより必ず弱く、連射の揺れ（上限 kFireTraumaLimit）の上に少し乗る上限にする。
	const float kSeverTrauma = 0.15f;
	const float kHeadBurstTrauma = 0.25f;
	const float kHeavyHitTraumaLimit = 0.6f;
	const float kTraumaFull = 1.f;

	// 揺れ方・ぶれ方を遊ぶたびに変える種（起動からの時間）。部品ごとに足す数を変えて、同じ並びにならないようにする。
	int timeSeed( int salt ) {
		return static_cast< int >( Framework::instance().time() ) + salt;
	}
	const int kAimRecoilSalt = 1;
	const int kAimPunchSalt = 2;
	const int kShakeSalt = 3;
}

ViewEffects::ViewEffects() :
	mAimRecoil( timeSeed( kAimRecoilSalt ) ),
	mAimPunch( timeSeed( kAimPunchSalt ) ),
	mShake( timeSeed( kShakeSalt ) ),
	mPendingLookYaw( 0.f ),
	mPendingLookPitch( 0.f ) {
}

void ViewEffects::addLook( float yawDegree, float pitchDegree ) {
	mPendingLookYaw += yawDegree;
	mPendingLookPitch += pitchDegree;
}

void ViewEffects::update( float shotRecoil, const AimSwayBody& body, FirstPersonCamera* camera ) {
	ASSERT( camera && "ViewEffects::update : camera is null." );
	mRecoil.update( shotRecoil );
	mAimRecoil.update( shotRecoil );
	mAimPunch.update();
	if ( shotRecoil > 0.f ) {
		mShake.addTrauma( kFireTrauma * shotRecoil, kFireTraumaLimit );
	}
	mShake.update();
	// ためたマウスの回転は、このティックで使い切る（1 回の描画に 2 ティックあれば、2 つ目は 0）。
	mSway.update( body, mPendingLookYaw, mPendingLookPitch );
	mPendingLookYaw = 0.f;
	mPendingLookPitch = 0.f;
	// 狙いのずれは 3 つを足して渡す（どれもカメラごと動き、弾の向きに入る）。揺れは描く行列だけへ。
	// 照準のぶれは銃と弾の向きだけへ（画面の中央は動かさない）。
	float yawOffset = mAimRecoil.yawOffset() + mAimPunch.yawOffset();
	float pitchOffset = mRecoil.pitchOffset() + mAimRecoil.pitchOffset() + mAimPunch.pitchOffset();
	camera->setAimOffset( yawOffset, pitchOffset );
	camera->setShake( mShake.yaw(), mShake.pitch(), mShake.roll() );
	camera->setWeaponSway( mSway.yawOffset(), mSway.pitchOffset() );
}

void ViewEffects::flinch() {
	mAimPunch.punch();
	mShake.addTrauma( kZombieHitTrauma, kTraumaFull );
}

void ViewEffects::shakeOnGrab() {
	mShake.addTrauma( kGrabTrauma, kTraumaFull );
}

void ViewEffects::shakeOnHeavyHit( bool headBurst ) {
	mShake.addTrauma( headBurst ? kHeadBurstTrauma : kSeverTrauma, kHeavyHitTraumaLimit );
}
