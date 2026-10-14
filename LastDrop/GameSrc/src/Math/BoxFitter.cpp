#include "GameSrc/Math/BoxFitter.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const int kAxisNumber = 3;
	const float kHalf = 0.5f;
	// これより短い向きは「向きが無い」とみなす（0 で割らない・向きの決まらない軸を使わないため）。
	const float kMinAxisLength = 0.000001f;
	// 広がりが一番大きい向きを求める反復（べき乗法）の回数。体の部位の点なら十分に収束する。
	const int kPowerIterations = 32;
	// 長い辺に直交する面の基準を作るときの補助の向き。長い辺とほぼ平行なら、もう一方を使う。
	const Vector3 kHelperUp( 0.f, 1.f, 0.f );
	const Vector3 kHelperSide( 1.f, 0.f, 0.f );
	const float kNearlyParallel = 0.9f;

	Vector3 meanOf( const std::vector< Vector3 >& points ) {
		Vector3 sum( 0.f, 0.f, 0.f );
		for ( const Vector3& p : points ) {
			sum += p;
		}
		sum *= 1.f / static_cast< float >( points.size() );
		return sum;
	}

	// 点の広がりが一番大きい向き（共分散行列の一番大きい固有値の固有ベクトル）をべき乗法で求める。
	// 広がりが無い（点が1か所に集まっている）なら kHelperUp を返す。
	Vector3 principalAxis( const std::vector< Vector3 >& points, const Vector3& mean ) {
		float covariance[ kAxisNumber ][ kAxisNumber ] = {};
		for ( const Vector3& p : points ) {
			Vector3 d;
			d.setSub( p, mean );
			for ( int i = 0; i < kAxisNumber; ++i ) {
				for ( int j = 0; j < kAxisNumber; ++j ) {
					covariance[ i ][ j ] += d[ i ] * d[ j ];
				}
			}
		}
		Vector3 axis( 1.f, 1.f, 1.f );
		axis.normalize();
		for ( int iteration = 0; iteration < kPowerIterations; ++iteration ) {
			Vector3 next( 0.f, 0.f, 0.f );
			for ( int i = 0; i < kAxisNumber; ++i ) {
				for ( int j = 0; j < kAxisNumber; ++j ) {
					next[ i ] += covariance[ i ][ j ] * axis[ j ];
				}
			}
			if ( next.length() < kMinAxisLength ) {
				return kHelperUp;
			}
			next.normalize();
			axis = next;
		}
		return axis;
	}

	// 長い辺 longAxis（長さ1）に直交する面の中で、点の広がりが一番大きい向き（長さ1）。
	// 面の上の2次元の共分散から、広がりが一番大きい角度を直接求める（2×2 の固有ベクトルは角度の式で出せる）。
	Vector3 crossSectionAxis( const std::vector< Vector3 >& points, const Vector3& mean, const Vector3& longAxis ) {
		const Vector3& helper = ( Math::abs( longAxis.dot( kHelperUp ) ) < kNearlyParallel ) ? kHelperUp : kHelperSide;
		Vector3 u;
		u.setCross( helper, longAxis );
		u.normalize();
		Vector3 v;
		v.setCross( longAxis, u );
		float uu = 0.f;
		float vv = 0.f;
		float uv = 0.f;
		for ( const Vector3& p : points ) {
			Vector3 d;
			d.setSub( p, mean );
			float pu = d.dot( u );
			float pv = d.dot( v );
			uu += pu * pu;
			vv += pv * pv;
			uv += pu * pv;
		}
		float angle = kHalf * Math::atan2( 2.f * uv, uu - vv ); // 度
		Vector3 axis;
		axis.setMul( u, Math::cos( angle ) );
		axis.madd( v, Math::sin( angle ) );
		axis.normalize();
		return axis;
	}
}

bool BoxFitter::fit( const std::vector< Vector3 >& points, const Vector3* longAxis, float minHalfExtent, Matrix34* boxOut ) {
	if ( points.empty() ) {
		return false;
	}
	Vector3 mean = meanOf( points );

	Vector3 axes[ kAxisNumber ];
	if ( longAxis && longAxis->length() >= kMinAxisLength ) {
		axes[ 0 ] = *longAxis;
		axes[ 0 ].normalize();
	} else {
		axes[ 0 ] = principalAxis( points, mean );
	}
	axes[ 1 ] = crossSectionAxis( points, mean, axes[ 0 ] );
	axes[ 2 ].setCross( axes[ 0 ], axes[ 1 ] );

	// 決めた向きごとに、点を落とした範囲を求める。
	Vector3 center( 0.f, 0.f, 0.f );
	float halfExtent[ kAxisNumber ];
	for ( int a = 0; a < kAxisNumber; ++a ) {
		float low = points[ 0 ].dot( axes[ a ] );
		float high = low;
		for ( const Vector3& p : points ) {
			float projected = p.dot( axes[ a ] );
			low = Math::min( low, projected );
			high = Math::max( high, projected );
		}
		center.madd( axes[ a ], ( low + high ) * kHalf );
		halfExtent[ a ] = Math::max( ( high - low ) * kHalf, minHalfExtent );
	}

	// 列 a が「a 番目の向き × 幅の半分」、移動が中心。-1〜1 の立方体がこの箱になる。
	Matrix34& m = *boxOut;
	m.m00 = axes[ 0 ].x * halfExtent[ 0 ]; m.m01 = axes[ 1 ].x * halfExtent[ 1 ]; m.m02 = axes[ 2 ].x * halfExtent[ 2 ]; m.m03 = center.x;
	m.m10 = axes[ 0 ].y * halfExtent[ 0 ]; m.m11 = axes[ 1 ].y * halfExtent[ 1 ]; m.m12 = axes[ 2 ].y * halfExtent[ 2 ]; m.m13 = center.y;
	m.m20 = axes[ 0 ].z * halfExtent[ 0 ]; m.m21 = axes[ 1 ].z * halfExtent[ 1 ]; m.m22 = axes[ 2 ].z * halfExtent[ 2 ]; m.m23 = center.z;
	return true;
}
