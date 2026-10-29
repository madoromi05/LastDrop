#include "GameSrc/Render/CascadedShadowMap.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Matrix34.h"
#include "GameLib/Math/Functions.h"

#include <cmath> // std::floor（GameLib の Math に無い）。三角関数は角度が度の GameLib の物を Math:: を付けて呼ぶ

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
	const float kNearestCascadeStart = 0.f; // 一番手前の枚は目から受け持つ
	// 光の向きがほぼ真上・真下のときは、世界の上を「上」にすると行列が作れないので、奥（+Z）を上にする（ShadowMap.cpp と同じ）。
	const float kNearlyVertical = 0.99f;
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kVerticalUp( 0.f, 0.f, 1.f );
	const Vector3 kOrigin( 0.f, 0.f, 0.f );
	const float kDepthNear = 0.f; // 正射影の手前の面は光の目の位置（見る向きの z は負なので、手前が 0・奥が負）

	const Vector3& lightUp( const Vector3& lightDirection ) {
		return ( Math::abs( lightDirection.y ) > kNearlyVertical ) ? kVerticalUp : kWorldUp;
	}
}

CascadedShadowMap::CascadedShadowMap( int size ) :
	mSize( size ) {
	for ( int i = 0; i < kCascadeCount; ++i ) {
		mMaps[ i ] = std::make_unique< ShadowMap >( size );
		mTexelSizes[ i ] = 0.f;
	}
}

void CascadedShadowMap::update( const Vector3& eye, const Vector3& viewDirection, float fieldOfViewY, float aspect,
	const Vector3& lightDirection, const float ( &cascadeEnds )[ kCascadeCount ], float casterMargin ) {
	// 画面の角の向きは、カメラの向きから「奥へ 1 進むと横へ lateral 離れる」。縦の tan と横の tan（縦 × 縦横比）の合成。
	float tanHalfY = Math::tan( fieldOfViewY * kHalf );
	float lateral = tanHalfY * Math::sqrt( 1.f + aspect * aspect );
	// 光の向きの基準（光から見た右と上）。原点から光の向きを見る行列の行が、そのまま右・上の向きになる。
	const Vector3& up = lightUp( lightDirection );
	Matrix34 basis;
	basis.setViewTransform( kOrigin, lightDirection, up );
	Vector3 right( basis.m00, basis.m01, basis.m02 );
	Vector3 upward( basis.m10, basis.m11, basis.m12 );
	float start = kNearestCascadeStart;
	for ( int i = 0; i < kCascadeCount; ++i ) {
		float end = cascadeEnds[ i ];
		// この枚が受け持つ「奥 start〜end の映る所」を包む球。中心は真ん中の奥、半径は中心から奥の角まで（奥の角が一番遠い）。
		float middle = ( start + end ) * kHalf;
		float halfDepth = ( end - start ) * kHalf;
		float farLateral = end * lateral;
		float radius = Math::sqrt( halfDepth * halfDepth + farLateral * farLateral );
		Vector3 center;
		center.setMadd( eye, viewDirection, middle );
		// 中心を影のマップの 1 画素の単位で丸める（光から見た右・上の向きの座標を丸め、その差だけ動かす）。
		float texel = radius * 2.f / static_cast< float >( mSize );
		float x = center.dot( right );
		float y = center.dot( upward );
		center.madd( right, std::floor( x / texel ) * texel - x );
		center.madd( upward, std::floor( y / texel ) * texel - y );
		// 光の目は、球より光の側へ casterMargin 離した所（球の外の高い建物の影も描けるように）。
		Vector3 lightEye;
		lightEye.setMadd( center, lightDirection, -( radius + casterMargin ) );
		Matrix34 view;
		view.setViewTransform( lightEye, center, up );
		Matrix44 projection;
		projection.setOrthogonalTransform( -radius, radius, -radius, radius, -( radius * 2.f + casterMargin ), kDepthNear );
		projection *= view;
		mProjectionViews[ i ] = projection;
		mTexelSizes[ i ] = texel;
		start = end;
	}
}

ShadowMap& CascadedShadowMap::map( int cascade ) {
	ASSERT( cascade >= 0 && cascade < kCascadeCount && "CascadedShadowMap : cascade out of range." );
	return *mMaps[ cascade ];
}

const Matrix44& CascadedShadowMap::projectionView( int cascade ) const {
	ASSERT( cascade >= 0 && cascade < kCascadeCount && "CascadedShadowMap : cascade out of range." );
	return mProjectionViews[ cascade ];
}

float CascadedShadowMap::texelSize( int cascade ) const {
	ASSERT( cascade >= 0 && cascade < kCascadeCount && "CascadedShadowMap : cascade out of range." );
	return mTexelSizes[ cascade ];
}

const char* CascadedShadowMap::errorMessage() const {
	return mMaps[ 0 ]->errorMessage(); // 3 枚とも同じシェーダ
}
