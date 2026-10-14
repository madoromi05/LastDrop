#include "GameSrc/Player/FirstPersonCamera.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 向き（度）──
	// 上下限は背骨の曲げで体が付いてこられる範囲。これより先は腰から折れたように見える。
	const float kMinPitch = -70.f; // 見上げる限界
	const float kMaxPitch = 70.f;  // 見下ろす限界

	// ── 射影 ──
	const float kDefaultFieldOfView = 60.f; // setFieldOfView を呼ぶまでの縦の視野角（度）
	// 自分の肩・腕・銃が目のすぐ前にあるので、TPS より近くまで描く（遠いと銃口や手が欠ける）。
	const float kNearClip     = 0.05f;
	const float kFarClip      = 1000.f;

	const float kFullTurn = 360.f;
	const Vector3 kUp( 0.f, 1.f, 0.f );
}

FirstPersonCamera::FirstPersonCamera() :
	mYaw( 0.f ),
	mPitch( 0.f ),
	mPitchOffset( 0.f ),
	mFieldOfView( kDefaultFieldOfView ),
	mEyePosition( 0.f, 0.f, 0.f ) {
}

void FirstPersonCamera::rotate( float yawDelta, float pitchDelta ) {
	mYaw += yawDelta;
	// ヨーは回し続けても桁あふれで精度が落ちないよう、1周で折り返す。
	if ( mYaw >= kFullTurn ) {
		mYaw -= kFullTurn;
	} else if ( mYaw < 0.f ) {
		mYaw += kFullTurn;
	}
	mPitch = Math::max( kMinPitch, Math::min( kMaxPitch, mPitch + pitchDelta ) );
}

void FirstPersonCamera::setPitchOffset( float degree ) {
	mPitchOffset = degree;
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
	Vector3 lookAt;
	lookAt.setAdd( mEyePosition, viewDirection() );

	Graphics::Manager gm = Graphics::Manager::instance();
	Matrix44 projectionView;
	projectionView.setPerspectiveTransform( mFieldOfView,
		static_cast< float >( gm.width() ),
		static_cast< float >( gm.height() ),
		kNearClip, kFarClip );
	Matrix34 view;
	view.setViewTransform( mEyePosition, lookAt, kUp );
	projectionView *= view;
	return projectionView;
}

float FirstPersonCamera::yaw() const {
	return mYaw;
}

float FirstPersonCamera::pitch() const {
	// 足した後も上下限で止める（真上近くで撃って、背骨が曲げられる範囲を越えないように）。
	return Math::max( kMinPitch, Math::min( kMaxPitch, mPitch + mPitchOffset ) );
}

Vector3 FirstPersonCamera::viewDirection() const {
	// 水平の前方（ヨー0で +Z）をピッチぶん下へ傾ける。
	float shownPitch = pitch(); // 反動などを足した向き
	return Vector3(
		Math::sin( mYaw ) * Math::cos( shownPitch ),
		-Math::sin( shownPitch ),
		Math::cos( mYaw ) * Math::cos( shownPitch ) );
}

const Vector3& FirstPersonCamera::eyePosition() const {
	return mEyePosition;
}
