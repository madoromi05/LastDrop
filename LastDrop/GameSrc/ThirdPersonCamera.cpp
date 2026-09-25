#include "ThirdPersonCamera.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 構図（メートル）──
	const float kDistance       = 3.5f;  // 注視点からカメラまでの距離
	const float kPivotHeight    = 1.5f;  // 注視点の高さ（足元から。身長約1.8の肩のあたり）
	// 注視点を右へずらして、キャラを画面の少し左に置く（肩越し）。進行方向の視界をキャラで塞がないため。
	const float kShoulderOffset = 0.5f;
	const float kMinEyeHeight   = 0.2f;  // 見上げても視点が地面(Y=0)の下へ潜らないようにする高さ

	// ── 向き（度）──
	const float kInitialPitch = 15.f;
	const float kMinPitch     = -30.f; // 見上げる限界
	const float kMaxPitch     = 70.f;  // 見下ろす限界（真上から見ると移動方向が分からなくなる）

	// ── 射影 ──
	const float kFieldOfViewY = 50.f;
	const float kNearClip     = 0.1f;  // 近くの壁やキャラの肩が切れないよう小さめ
	const float kFarClip      = 1000.f;

	const Vector3 kUp( 0.f, 1.f, 0.f );
}

ThirdPersonCamera::ThirdPersonCamera() :
	mYaw( 0.f ),
	mPitch( kInitialPitch ),
	mEyePosition( 0.f, 0.f, 0.f ),
	mLookAt( 0.f, 0.f, 0.f ) {
}

void ThirdPersonCamera::rotate( float yawDelta, float pitchDelta ) {
	mYaw += yawDelta;
	// ヨーは回し続けても桁あふれで精度が落ちないよう、1周で折り返す。
	const float kFullTurn = 360.f;
	if ( mYaw >= kFullTurn ) {
		mYaw -= kFullTurn;
	} else if ( mYaw < 0.f ) {
		mYaw += kFullTurn;
	}
	mPitch = Math::max( kMinPitch, Math::min( kMaxPitch, mPitch + pitchDelta ) );
}

void ThirdPersonCamera::follow( const Vector3& targetFootPosition ) {
	// 水平の前方と右。前方は setAngleY と同じ約束（ヨー0で +Z）、右は 前方×上。
	Vector3 forward( Math::sin( mYaw ), 0.f, Math::cos( mYaw ) );
	Vector3 right;
	right.setCross( forward, kUp );

	// 注視点: 足元から肩の高さへ上げ、右へずらす。
	mLookAt = targetFootPosition;
	mLookAt.y += kPivotHeight;
	mLookAt.madd( right, kShoulderOffset );

	// 視線: ピッチぶん下を向く。視点は注視点から視線の逆向きへ kDistance 下がった所。
	Vector3 viewDirection(
		forward.x * Math::cos( mPitch ),
		-Math::sin( mPitch ),
		forward.z * Math::cos( mPitch ) );
	mEyePosition.setMadd( mLookAt, viewDirection, -kDistance );
	if ( mEyePosition.y < kMinEyeHeight ) {
		mEyePosition.y = kMinEyeHeight;
	}
}

void ThirdPersonCamera::apply() const {
	Graphics::Manager gm = Graphics::Manager::instance();
	Matrix44 projectionView;
	projectionView.setPerspectiveTransform( kFieldOfViewY,
		static_cast< float >( gm.width() ),
		static_cast< float >( gm.height() ),
		kNearClip, kFarClip );
	Matrix34 view;
	view.setViewTransform( mEyePosition, mLookAt, kUp );
	projectionView *= view;
	gm.setProjectionViewMatrix( projectionView );
	gm.setEyePosition( mEyePosition );
}

float ThirdPersonCamera::yaw() const {
	return mYaw;
}

const Vector3& ThirdPersonCamera::eyePosition() const {
	return mEyePosition;
}
