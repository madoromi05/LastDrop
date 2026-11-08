#include "GameSrc/Player/Body/Pose/WeaponPlacePose.h"
#include "GameSrc/Player/Body/Pose/FirstPersonRig.h"
#include "GameSrc/Player/Body/Pose/PoseMath.h"
#include "GameSrc/Player/Body/WeaponPoseParams.h"
#include "GameSrc/Math/AxisRotation.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 手首から見た2つの向きがこれより平行に近ければ、回転軸が決まらないので回さない（ほぼ回す必要も無い）。
	const float kMinAxisLength = 1e-6f;
}

WeaponPlacePose::WeaponPlacePose( const FirstPersonRig* rig ) :
	mRig( rig ),
	mHoldWeight( 0.f ),
	mCarryWeight( 0.f ),
	mAimWeight( 0.f ) {
	ASSERT( mRig && "WeaponPlacePose : rig is null." );
}

void WeaponPlacePose::setHoldWeight( float weight ) {
	mHoldWeight = weight;
}

void WeaponPlacePose::setCarryWeight( float weight ) {
	mCarryWeight = weight;
}

void WeaponPlacePose::setAimWeight( float weight ) {
	mAimWeight = weight;
}

void WeaponPlacePose::apply( std::vector< Matrix34 >* pose ) {
	if ( !mRig->hasWeapon() ) {
		return;
	}
	holdWithBothHands( pose );
	carryAtChest( pose );
	alignSightToEye( pose );
}

void WeaponPlacePose::holdWithBothHands( std::vector< Matrix34 >* pose ) const {
	if ( mRig->supportHandBone < 0 || mHoldWeight <= 0.f ) {
		return;
	}
	// 手首（右手の骨の原点）から見た「銃の上で左手が来るべき点」と「今の左手」の向きを重ねる、最小の回転を求める。
	// 手首を中心に回すのは、握りの中心を中心にすると手首が前腕から外れて見えるため。
	Matrix34 gun = mRig->weaponPlacement( *pose );
	Vector3 supportOnGun;
	gun.mul( &supportOnGun, mRig->supportHandOnWeapon );
	Vector3 wrist = PlacementMatrix::translationOf( ( *pose )[ mRig->handBone ] );
	Vector3 toGun;
	toGun.setSub( supportOnGun, wrist );
	Vector3 toHand;
	toHand.setSub( PlacementMatrix::translationOf( ( *pose )[ mRig->supportHandBone ] ), wrist );

	Vector3 axis;
	axis.setCross( toGun, toHand );
	float sine = axis.length();
	if ( sine < kMinAxisLength ) {
		return;
	}
	axis *= 1.f / sine;
	float angle = Math::atan2( sine, toGun.dot( toHand ) ); // 度。外積の長さと内積の比から求めると、0°・180°付近でも精度が落ちない
	// 手首から左手までの距離は合わせない（向きだけ）。向きを合わせた後の残りは 歩き 1〜1.7cm・走り 0.6cm 以下と測れたので、
	// 腕を伸び縮みさせる処理（IK）は持たない。
	PoseMath::transformBones( AxisRotation::about( wrist, axis, angle * mHoldWeight ), mRig->rightHandBones, pose );
}

void WeaponPlacePose::carryAtChest( std::vector< Matrix34 >* pose ) const {
	if ( mRig->chestBone < 0 || mCarryWeight <= 0.f ) {
		return;
	}
	const WeaponPoseParams& params = weaponPoseParams();
	Matrix34 gun = mRig->weaponPlacement( *pose );
	// 胸の向きは、胸の骨の回転ではなく体の前方をピッチぶん傾けたものにする（背骨はピッチぶん曲げてある）。
	// 胸の骨の回転は走りのアニメで大きく揺れるので、そのまま使うと銃が画面の中で暴れる。位置の揺れ（上下）だけを銃に出す。
	Matrix34 chest = mRig->viewRotation();
	Vector3 wristGoal;
	chest.mul33( &wristGoal, params.carryWristFromChest );
	wristGoal += PlacementMatrix::translationOf( ( *pose )[ mRig->chestBone ] );
	Matrix34 carry = chest;
	carry.rotateY( params.carryMuzzleInward );
	carry.rotateX( params.carryMuzzleDown );
	carry.rotateZ( params.carryCant );
	// 置き場所を手首で決めるのは、手首の位置と銃の向きを別々の定数で合わせられるようにするため（銃の先は向きだけで決まる）。
	Vector3 wrist = PlacementMatrix::translationOf( ( *pose )[ mRig->handBone ] );
	PoseMath::transformBonesPartially( PoseMath::alignWeapon( gun, carry, wrist, wristGoal ), mCarryWeight, mRig->armBones, pose );
}

void WeaponPlacePose::alignSightToEye( std::vector< Matrix34 >* pose ) const {
	if ( mAimWeight <= 0.f ) {
		return;
	}
	// 今のポーズでの銃の置き場所。
	Matrix34 gun = mRig->weaponPlacement( *pose );
	// 視線の3軸。前方 +Z をピッチぶん下へ傾けたもの（FirstPersonCamera と同じ向き）。
	// 銃をこの向きに揃えると、銃の傾き（カント）も取れてサイトがまっすぐ立つ。
	// サイトの目の点を目の位置へ置く。
	Vector3 sight;
	gun.mul( &sight, mRig->sightEye );
	PoseMath::transformBonesPartially( PoseMath::alignWeapon( gun, mRig->viewRotation(), sight, mRig->eyePosition ), mAimWeight, mRig->armBones, pose );
}
