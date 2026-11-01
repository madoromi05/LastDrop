#include "GameSrc/Render/DrawCulling.h"
#include "GameSrc/Hit/HitboxSet.h"
#include "GameSrc/Math/ScalarMath.h"

#include "GameLib/Math/Functions.h"
#include "GameLib/Profiling/Profiler.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 画面（影のマップ）の上でこれより小さい物は描かない（画素。Q7）。
	const float kMinPixels = 2.f;
	// 霧の後に残る色がこの段の数の 1 段より少なければ、見えないとみなす（8bit の色の 1 段 ＝ 1/255。Q6）。
	const float kColorSteps = 255.f;
	const float kHalf = 0.5f;
	const float kNoLimit = 0.f; // 距離・小ささを見ない
}

const float DrawCulling::kPoseMargin = 0.3f;

DrawCulling::DrawCulling( const Matrix44& projectionView ) :
	DrawCulling( projectionView, Vector3( 0.f, 0.f, 0.f ), kNoLimit, kNoLimit, kNoLimit ) {
}

DrawCulling::DrawCulling( const Matrix44& projectionView, const Vector3& eye, float drawDistance, float minRadiusPerDistance,
	float minDiameter ) :
	mFrustum( projectionView ),
	mEye( eye ),
	mDrawDistance( drawDistance ),
	mMinRadiusPerDistance( minRadiusPerDistance ),
	mMinDiameter( minDiameter ) {
}

DrawCulling DrawCulling::forCamera( const Matrix44& projectionView, const Vector3& eye, float drawDistance, float fieldOfViewY,
	int screenHeight ) {
	// 画面の上の半径（画素）＝ 半径 ÷ 距離 × ( 画面の高さ ÷ 2 ) ÷ tan( 縦の画角 ÷ 2 )。これが kMinPixels になる「半径 ÷ 距離」。
	float pixelsPerRadiusPerDistance = static_cast< float >( screenHeight ) * kHalf / Math::tan( fieldOfViewY * kHalf );
	return DrawCulling( projectionView, eye, drawDistance, kMinPixels / pixelsPerRadiusPerDistance, kNoLimit );
}

DrawCulling DrawCulling::forOrthographicShadow( const Matrix44& projectionView, float texelSize ) {
	return DrawCulling( projectionView, Vector3( 0.f, 0.f, 0.f ), kNoLimit, kNoLimit, kMinPixels * texelSize );
}

float DrawCulling::drawDistanceForFog( float fogHalfDistance ) {
	// 距離の霧で残る割合は exp( −( d × √ln2 ÷ 半分の距離 )² ) ＝ 2^( −( d ÷ 半分の距離 )² )（Fog.hlsli・LightList::setEnvironment）。
	// これが 1 ÷ kColorSteps になる d ＝ 半分の距離 × √( log2( kColorSteps ) )（半分の距離 40m なら約 113m）。
	return fogHalfDistance * Math::sqrt( Math::log2( kColorSteps ) );
}

bool DrawCulling::shows( const Vector3& boundsMin, const Vector3& boundsMax, Target target ) const {
	if ( !mFrustum.mayContain( boundsMin, boundsMax ) ) {
		if ( target == Target::Zombie ) {
			PROFILE_COUNT( "Cull.zombie", 1 );
		}
		return false;
	}
	// 目から箱の一番近い所までの距離（目が箱の中なら 0）。遠さも小ささも、一番大きく見える所で決める（見える物を消さない側）。
	Vector3 nearest(
		ScalarMath::clamp( mEye.x, boundsMin.x, boundsMax.x ),
		ScalarMath::clamp( mEye.y, boundsMin.y, boundsMax.y ),
		ScalarMath::clamp( mEye.z, boundsMin.z, boundsMax.z ) );
	Vector3 toNearest;
	toNearest.setSub( nearest, mEye );
	float distance = toNearest.length();
	if ( mDrawDistance > kNoLimit && distance > mDrawDistance ) {
		count( "Cull.distance", target );
		return false;
	}
	if ( target != Target::Small ) {
		return true;
	}
	Vector3 size;
	size.setSub( boundsMax, boundsMin );
	float diameter = size.length();
	if ( diameter < mMinDiameter || diameter * kHalf < mMinRadiusPerDistance * distance ) {
		count( "Cull.small", target );
		return false;
	}
	return true;
}

bool DrawCulling::showsPosed( const HitboxSet& hitboxes, Target target ) const {
	Vector3 boundsMin;
	Vector3 boundsMax;
	if ( !hitboxes.bounds( &boundsMin, &boundsMax ) ) {
		return true;
	}
	const Vector3 margin( kPoseMargin, kPoseMargin, kPoseMargin );
	boundsMin -= margin;
	boundsMax += margin;
	return shows( boundsMin, boundsMax, target );
}

const ViewFrustum& DrawCulling::frustum() const {
	return mFrustum;
}

void DrawCulling::count( const char* reason, Target target ) {
	PROFILE_COUNT( reason, 1 );
	if ( target == Target::Zombie ) {
		PROFILE_COUNT( "Cull.zombie", 1 );
	}
}
