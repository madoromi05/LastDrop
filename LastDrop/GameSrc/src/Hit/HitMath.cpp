#include "GameSrc/Hit/HitMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kCubeHalf = 1.f;           // 立方体の幅の半分（-1〜1）
	const float kMinMove = 0.000001f;      // これより小さい移動は、その軸には動かないとみなす（0 割りを避ける）
	const float kStartTime = 0.f;
	const float kEndTime = 1.f;
	const int kAxisNumber = 3;
	const int kNoAxis = -1;                // 入った面が無い（始点が中）
	const float kOutward = 1.f;
	// 三角形と線分がほぼ平行・三角形の面積がほぼ0とみなす大きさ（行列式）。
	const float kParallelEpsilon = 1e-12f;

	// 軸ごとの「入る時刻・出る時刻」を重ねていく（スラブ法）。重なりが無くなれば外れ。
	// low/high: その軸の幅。enterAxis: 一番遅く入った軸（面の法線に使う）。nullptr なら記録しない。
	bool clipAxis( float start, float move, float low, float high, int axis,
	float* enterTime, float* exitTime, int* enterAxis, float* enterSign ) {
		if ( Math::abs( move ) < kMinMove ) {
			return ( start >= low && start <= high ); // この軸には動かない。幅の中にいれば、この軸は制限しない
		}
		float toLow = ( low - start ) / move;
		float toHigh = ( high - start ) / move;
		float enter = Math::min( toLow, toHigh );
		float exit = Math::max( toLow, toHigh );
		if ( enter > *enterTime ) {
			*enterTime = enter;
			if ( enterAxis ) {
				*enterAxis = axis;
				// 正の向きに進んで入ったなら、入った面は低い側（外向きの法線は負）。
				*enterSign = ( move > 0.f ) ? -kOutward : kOutward;
			}
		}
		*exitTime = Math::min( *exitTime, exit );
		return *enterTime <= *exitTime;
	}
}

bool HitMath::segmentUnitCube( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) {
	float enterTime = kStartTime; // 線分の始点より前は見ない（始点が中なら 0 のまま）
	float exitTime = kEndTime;
	int enterAxis = kNoAxis;
	float enterSign = kOutward;
	for ( int axis = 0; axis < kAxisNumber; ++axis ) {
		if ( !clipAxis( origin[ axis ], delta[ axis ], -kCubeHalf, kCubeHalf, axis, &enterTime, &exitTime, &enterAxis, &enterSign ) ) {
			return false;
		}
	}
	*time = enterTime;
	normal->set( 0.f, 0.f, 0.f );
	if ( enterAxis != kNoAxis ) {
		( *normal )[ enterAxis ] = enterSign;
	}
	return true;
}

bool HitMath::segmentTouchesBox( const Vector3& origin, const Vector3& delta, const Vector3& boundsMin, const Vector3& boundsMax ) {
	float enterTime;
	return segmentBoxEnterTime( origin, delta, boundsMin, boundsMax, &enterTime );
}

bool HitMath::segmentBoxEnterTime( const Vector3& origin, const Vector3& delta, const Vector3& boundsMin, const Vector3& boundsMax,
float* enterTime ) {
	float enter = kStartTime;
	float exit = kEndTime;
	for ( int axis = 0; axis < kAxisNumber; ++axis ) {
		if ( !clipAxis( origin[ axis ], delta[ axis ], boundsMin[ axis ], boundsMax[ axis ], axis, &enter, &exit, nullptr, nullptr ) ) {
			return false;
		}
	}
	*enterTime = enter;
	return true;
}

bool HitMath::segmentTriangle( const Vector3& origin, const Vector3& delta,
const Vector3& v0, const Vector3& v1, const Vector3& v2, float* time, Vector3* normal ) {
	// Möller–Trumbore 法。当たる点を「v0 + u*edge1 + v*edge2」と「origin + t*delta」の両方で表して解く。
	Vector3 edge1;
	Vector3 edge2;
	edge1.setSub( v1, v0 );
	edge2.setSub( v2, v0 );
	Vector3 p;
	p.setCross( delta, edge2 );
	float determinant = edge1.dot( p );
	if ( Math::abs( determinant ) < kParallelEpsilon ) {
		return false; // 線分が面と平行、または面積0の三角形
	}
	float inverse = 1.f / determinant;
	Vector3 toOrigin;
	toOrigin.setSub( origin, v0 );
	float u = toOrigin.dot( p ) * inverse;
	if ( u < 0.f || u > 1.f ) {
		return false;
	}
	Vector3 q;
	q.setCross( toOrigin, edge1 );
	float v = delta.dot( q ) * inverse;
	if ( v < 0.f || u + v > 1.f ) {
		return false;
	}
	float t = edge2.dot( q ) * inverse;
	if ( t < kStartTime || t > kEndTime ) {
		return false;
	}
	*time = t;
	normal->setCross( edge1, edge2 );
	normal->normalize();
	// 両面とも当たるので、線分が来た側を向ける（弾痕を貼る向き・跳ね返りに使うため）。
	if ( normal->dot( delta ) > 0.f ) {
		normal->setNeg( *normal );
	}
	return true;
}

void HitMath::normalToWorld( const Matrix34& toLocal, const Vector3& localNormal, Vector3* out ) {
	// 置いた行列 M の法線は (M^-1)^T で移す。M^-1 = toLocal なので、その転置を掛ける（移動は掛けない）。
	out->set(
		toLocal.m00 * localNormal.x + toLocal.m10 * localNormal.y + toLocal.m20 * localNormal.z,
		toLocal.m01 * localNormal.x + toLocal.m11 * localNormal.y + toLocal.m21 * localNormal.z,
		toLocal.m02 * localNormal.x + toLocal.m12 * localNormal.y + toLocal.m22 * localNormal.z );
	out->normalize();
}
