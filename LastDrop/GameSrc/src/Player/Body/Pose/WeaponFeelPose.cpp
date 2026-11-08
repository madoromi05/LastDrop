#include "GameSrc/Player/Body/Pose/WeaponFeelPose.h"
#include "GameSrc/Player/Body/Pose/FirstPersonRig.h"
#include "GameSrc/Player/Body/Pose/PoseMath.h"
#include "GameSrc/Player/Body/WeaponPoseParams.h"
#include "GameSrc/Player/Camera/AimSwayParams.h"
#include "GameSrc/Math/AxisRotation.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

WeaponFeelPose::WeaponFeelPose( const FirstPersonRig* rig ) :
	mRig( rig ),
	mSwayYaw( 0.f ),
	mSwayPitch( 0.f ),
	mRecoil( 0.f ),
	mAimWeight( 0.f ) {
	ASSERT( mRig && "WeaponFeelPose : rig is null." );
}

void WeaponFeelPose::setSway( float yawDegree, float pitchDegree ) {
	mSwayYaw = yawDegree;
	mSwayPitch = pitchDegree;
}

void WeaponFeelPose::setRecoil( float kick ) {
	mRecoil = kick;
}

void WeaponFeelPose::setAimWeight( float weight ) {
	mAimWeight = weight;
}

void WeaponFeelPose::apply( std::vector< Matrix34 >* pose ) {
	if ( !mRig->hasWeapon() ) {
		return;
	}
	swayWeapon( pose );
	kickWeapon( pose );
}

void WeaponFeelPose::swayWeapon( std::vector< Matrix34 >* pose ) const {
	if ( mSwayYaw == 0.f && mSwayPitch == 0.f ) {
		return;
	}
	// 回す中心は目から視線の逆へ下げた所（視線は前方 +Z をピッチぶん下へ傾けたもの。覗く補正と同じ向き）。
	Vector3 behind;
	mRig->viewRotation().mul33( &behind, Vector3( 0.f, 0.f, -aimSwayParams().pivotBehindEye ) );
	Vector3 pivot;
	pivot.setAdd( mRig->eyePosition, behind );
	// 体はカメラのヨーを向き、背骨はピッチぶん曲げてあるので、銃の向きは「ヨー 0・ピッチ pitch」の視線に揃っている。
	// そこへ上の軸（Y）まわりにヨー、横の軸（X）まわりにピッチを足すと、向きは ヨー mSwayYaw・ピッチ pitch + mSwayPitch になり、
	// FirstPersonCamera::weaponDirection と同じ向きになる（X まわりの回転どうしは順番を入れ替えても同じなので、ピッチを先に戻さなくてよい）。
	Matrix34 sway; // 中心を原点へ → 回す → 中心へ戻す
	sway.setTranslation( pivot );
	sway.rotateY( mSwayYaw );
	sway.rotateX( mSwayPitch );
	sway.translate( -pivot.x, -pivot.y, -pivot.z );
	PoseMath::transformBones( sway, mRig->armBones, pose );
}

void WeaponFeelPose::kickWeapon( std::vector< Matrix34 >* pose ) const {
	if ( mRecoil <= 0.f ) {
		return;
	}
	const WeaponPoseParams& params = weaponPoseParams();
	// 今の銃の向き（銃口 = モデルの +Z、横 = +X）。
	Vector3 barrel;
	Vector3 gunSide;
	PoseMath::weaponAxes( mRig->weaponPlacement( *pose ), &barrel, &gunSide );

	// 覗き具合で、腰だめの動きと覗いているときの動きの間を取る。
	float back = ( params.recoilBackHip + ( params.recoilBackAim - params.recoilBackHip ) * mAimWeight ) * mRecoil;
	float rise = ( params.recoilMuzzleRiseHip + ( params.recoilMuzzleRiseAim - params.recoilMuzzleRiseHip ) * mAimWeight ) * mRecoil;

	// 手首を中心に銃口を跳ね上げてから、銃口と逆の向きへ引く。銃の横の軸まわりは、正の角度で銃口が下を向くので負で回す。
	// 手首を中心にするのは、握っている所を支点に銃の先が跳ねるように見せるため。
	Matrix34 kick = AxisRotation::about( PlacementMatrix::translationOf( ( *pose )[ mRig->handBone ] ), gunSide, -rise );
	kick.m03 -= barrel.x * back;
	kick.m13 -= barrel.y * back;
	kick.m23 -= barrel.z * back;
	PoseMath::transformBones( kick, mRig->armBones, pose );
}
