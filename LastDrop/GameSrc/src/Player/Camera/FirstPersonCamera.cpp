#include "GameSrc/Player/Camera/FirstPersonCamera.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 向き（度）──
	// 上下限は背骨の曲げで体が付いてこられる範囲。これより先は腰から折れたように見える
	const float kBodyPitchLimit = 70.f; // 見上げる・見下ろす限界

	// ── 射影 ──
	const float kDefaultFieldOfView = 60.f; // setFieldOfView を呼ぶまでの縦の視野角（度）
	// 自分の肩・腕・銃が目のすぐ前にあるので、TPS より近くまで描く（遠いと銃口や手が欠ける）
	const float kNearClip     = 0.05f;
	const float kFarClip      = 1000.f;

	const float kFullTurn = 360.f;
	const Vector3 kUp( 0.f, 1.f, 0.f );

	// ヨー・ピッチ（度）から向き（長さ1）。ヨー0で +Z、ピッチは正で見下ろす
	Vector3 directionOf( float yaw, float pitch ) {
		return Vector3(
			Math::sin( yaw ) * Math::cos( pitch ),
			-Math::sin( pitch ),
			Math::cos( yaw ) * Math::cos( pitch ) );
	}
}

FirstPersonCamera::FirstPersonCamera() :
	FirstPersonCamera( kBodyPitchLimit ) {
}

FirstPersonCamera::FirstPersonCamera( float pitchLimit ) :
	mPitchLimit( pitchLimit ),
	mYaw( 0.f ),
	mPitch( 0.f ),
	mYawOffset( 0.f ),
	mPitchOffset( 0.f ),
	mShakeYaw( 0.f ),
	mShakePitch( 0.f ),
	mShakeRoll( 0.f ),
	mSwayYaw( 0.f ),
	mSwayPitch( 0.f ),
	mFieldOfView( kDefaultFieldOfView ),
	mEyePosition( 0.f, 0.f, 0.f ) {
}

void FirstPersonCamera::rotate( float yawDelta, float pitchDelta ) {
	mYaw += yawDelta;
	// ヨーは回し続けても桁あふれで精度が落ちないよう、1周で折り返す
	if ( mYaw >= kFullTurn ) {
		mYaw -= kFullTurn;
	} else if ( mYaw < 0.f ) {
		mYaw += kFullTurn;
	}
	mPitch = Math::max( -mPitchLimit, Math::min( mPitchLimit, mPitch + pitchDelta ) );
}

void FirstPersonCamera::setAimOffset( float yawDegree, float pitchDegree ) {
	mYawOffset = yawDegree;
	mPitchOffset = pitchDegree;
}

void FirstPersonCamera::setShake( float yawDegree, float pitchDegree, float rollDegree ) {
	mShakeYaw = yawDegree;
	mShakePitch = pitchDegree;
	mShakeRoll = rollDegree;
}

void FirstPersonCamera::setWeaponSway( float yawDegree, float pitchDegree ) {
	mSwayYaw = yawDegree;
	mSwayPitch = pitchDegree;
}

void FirstPersonCamera::setEyePosition( const Vector3& eye ) {
	mEyePosition = eye;
}

void FirstPersonCamera::setFieldOfView( float degree ) {
	mFieldOfView = degree;
}

void FirstPersonCamera::apply() const {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setProjectionViewMatrix( projectionViewMatrix() );
	gm.setEyePosition( mEyePosition );
}

Matrix44 FirstPersonCamera::projectionViewMatrix() const {
	// 揺れは描く行列にだけ入れる（狙いの向き viewDirection は揺らさない）
	// 揺れたピッチは上下限で止めない（限界の所で撃っても揺れが消えないように。揺れは 1° 程度なので体には影響しない）
	Vector3 lookAt;
	lookAt.setAdd( mEyePosition, directionOf( yaw() + mShakeYaw, pitch() + mShakePitch ) );

	Graphics::Manager gm = Graphics::Manager::instance();
	Matrix44 projectionView;
	projectionView.setPerspectiveTransform( mFieldOfView,
		static_cast< float >( gm.width() ),
		static_cast< float >( gm.height() ),
		kNearClip, kFarClip );
	Matrix34 view;
	view.setViewTransform( mEyePosition, lookAt, kUp );
	// ロールは視点から見た奥行きの軸（Z）の周りに回す。ビュー変換の後に掛けるので、視線の向きは変わらない
	Matrix34 roll;
	roll.setRotationZ( mShakeRoll );
	Matrix34 rolledView;
	rolledView.setMul( roll, view );
	projectionView *= rolledView;
	return projectionView;
}

float FirstPersonCamera::yaw() const {
	return mYaw + mYawOffset;
}

float FirstPersonCamera::pitch() const {
	// 足した後も上下限で止める（真上近くで撃って、背骨が曲げられる範囲を越えないように）
	return Math::max( -mPitchLimit, Math::min( mPitchLimit, mPitch + mPitchOffset ) );
}

Vector3 FirstPersonCamera::viewDirection() const {
	return directionOf( yaw(), pitch() ); // 反動・怯みを足した向き
}

Vector3 FirstPersonCamera::weaponDirection() const {
	// ぶれたピッチは上下限で止めない（銃の見た目は FirstPersonBody が同じ角度で回していて、そちらも止めていないため）
	return directionOf( yaw() + mSwayYaw, pitch() + mSwayPitch );
}

float FirstPersonCamera::weaponSwayYaw() const {
	return mSwayYaw;
}

float FirstPersonCamera::weaponSwayPitch() const {
	return mSwayPitch;
}

float FirstPersonCamera::fieldOfView() const {
	return mFieldOfView;
}

const Vector3& FirstPersonCamera::eyePosition() const {
	return mEyePosition;
}
