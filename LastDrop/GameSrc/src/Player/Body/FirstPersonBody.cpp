#include "GameSrc/Player/Body/FirstPersonBody.h"
#include "GameSrc/Player/Body/WeaponPoseParams.h"

using namespace GameLib::Math;

namespace{
	const int kNoBone = -1; // 銃を持つ手が無い（どの銃の段も、これで止まる）

	WeaponPoseParams gParams;	// 抱え方・反動・目の位置の調整値（起動中に F6 で動かせる）。読むのは Pose/ の段（weaponPoseParams() で引く）
}

WeaponPoseParams& weaponPoseParams() {
	return gParams;
}

FirstPersonBody::FirstPersonBody( const SkinnedMeshData* data ) :
	mRig( data ),
	mArmOverride( &mRig ),
	mSpineBend( &mRig ),
	mEyePoint( &mRig ),
	mWeaponPlace( &mRig ),
	mWeaponFeel( &mRig ),
	mWeaponBounds( &mRig ) {
	// 順番に意味がある（それぞれの段のヘッダ）：
	// 腕の差し替えを最初にするのは、差し替えた腕も背骨の曲げ（視線の上下）に付いてくるようにするため
	// 目は曲げた後の頭で決める（曲げる前だと、見上げ・見下ろしで目と銃がずれる）
	// ぶれと反動は、置き終えた銃（覗いていればサイトが目に合った銃）から動かす。覗く補正より前に掛けると、合わせ直されて消える
	// 銃を囲むのは最後に銃を動かした後（見えている銃と当たり判定がずれないように）
	mChain.add( &mArmOverride );
	mChain.add( &mSpineBend );
	mChain.add( &mEyePoint );
	mChain.add( &mWeaponPlace );
	mChain.add( &mWeaponFeel );
	mChain.add( &mWeaponBounds );
}

void FirstPersonBody::setPitch( float degree ) {
	mRig.pitch = degree;
}

void FirstPersonBody::setWeapon( int handBone, const Matrix34& grip, const Vector3& sightEye,
int supportHandBone, const Vector3& supportHandOnWeapon, const Vector3& weaponMin, const Vector3& weaponMax ) {
	mRig.handBone = handBone;
	mRig.grip = grip;
	mRig.sightEye = sightEye;
	mRig.supportHandBone = supportHandBone;
	mRig.supportHandOnWeapon = supportHandOnWeapon;
	mRig.weaponMin = weaponMin;
	mRig.weaponMax = weaponMax;
	mWeaponBounds.reset(); // 次の apply で、この銃の範囲を求め直す
}

void FirstPersonBody::clearWeapon() {
	mRig.handBone = kNoBone; // 銃を持つ手が無い（どの銃の段も hasWeapon で止まる）
	mRig.supportHandBone = kNoBone;
	mWeaponBounds.reset();
}

void FirstPersonBody::setHoldWeight( float weight ) {
	mWeaponPlace.setHoldWeight( weight );
}

void FirstPersonBody::setAimWeight( float weight ) {
	// 覗き具合は 2 つの段が読む（サイトを目へ寄せる・反動の大きさを腰だめと覗くの間で取る）
	mWeaponPlace.setAimWeight( weight );
	mWeaponFeel.setAimWeight( weight );
}

void FirstPersonBody::setCarryWeight( float weight ) {
	mWeaponPlace.setCarryWeight( weight );
}

void FirstPersonBody::setRecoil( float kick ) {
	mWeaponFeel.setRecoil( kick );
}

void FirstPersonBody::setSway( float yawDegree, float pitchDegree ) {
	mWeaponFeel.setSway( yawDegree, pitchDegree );
}

void FirstPersonBody::setArmOverride( const std::vector< Matrix34 >* pose, float weight, float followHandWeight, const Matrix34* followReference ) {
	mArmOverride.set( pose, weight, followHandWeight, followReference );
}

const Vector3& FirstPersonBody::eyePosition() const {
	return mRig.eyePosition;
}

bool FirstPersonBody::weaponBounds( Vector3* low, Vector3* high ) const {
	return mWeaponBounds.bounds( low, high );
}

void FirstPersonBody::apply( std::vector< Matrix34 >* pose ) {
	mChain.apply( pose );
}
