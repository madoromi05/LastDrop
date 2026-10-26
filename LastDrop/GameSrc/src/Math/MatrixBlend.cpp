#include "GameSrc/Math/MatrixBlend.h"
#include "GameSrc/Math/ScalarMath.h"

#include <cmath>

using namespace GameLib::Math;

namespace{
	const int kAxisNumber = 3;
	const float kMinScale = 1e-8f;           // 軸の長さがこれ未満なら潰れた行列とみなし、割り算しない
	const float kNlerpThreshold = 0.9995f;   // 2つの回転がこれ以上近いと slerp の sin が0に近づくので、正規化線形補間で代用

	// 回転を表す単位四元数。このファイルの中だけで使う（GameLib に四元数が無いため）。
	struct Quaternion{
		float x, y, z, w;
	};

	// 行列の列ベクトル（＝ローカルの x/y/z 軸がワールドでどちらを向くか）。
	// Matrix34 は「列ベクトルに右から掛ける」形で、m03/m13/m23 が移動。
	float element( const Matrix34& m, int row, int column ) {
		return m( row, column );
	}

	float axisLength( const Matrix34& m, int column ) {
		float x = element( m, 0, column );
		float y = element( m, 1, column );
		float z = element( m, 2, column );
		return std::sqrt( x * x + y * y + z * z );
	}

	// 拡大を取り除いた回転行列（3x3 部分）から四元数を作る。
	// 対角成分の大きいものを基準にして、sqrt の中身が小さくなって精度が落ちるのを避ける（定番の方法）。
	Quaternion toQuaternion( const float r[ kAxisNumber ][ kAxisNumber ] ) {
		Quaternion q;
		float trace = r[ 0 ][ 0 ] + r[ 1 ][ 1 ] + r[ 2 ][ 2 ];
		if ( trace > 0.f ) {
			float s = std::sqrt( trace + 1.f ) * 2.f;
			q.w = 0.25f * s;
			q.x = ( r[ 2 ][ 1 ] - r[ 1 ][ 2 ] ) / s;
			q.y = ( r[ 0 ][ 2 ] - r[ 2 ][ 0 ] ) / s;
			q.z = ( r[ 1 ][ 0 ] - r[ 0 ][ 1 ] ) / s;
		} else if ( r[ 0 ][ 0 ] > r[ 1 ][ 1 ] && r[ 0 ][ 0 ] > r[ 2 ][ 2 ] ) {
			float s = std::sqrt( 1.f + r[ 0 ][ 0 ] - r[ 1 ][ 1 ] - r[ 2 ][ 2 ] ) * 2.f;
			q.w = ( r[ 2 ][ 1 ] - r[ 1 ][ 2 ] ) / s;
			q.x = 0.25f * s;
			q.y = ( r[ 0 ][ 1 ] + r[ 1 ][ 0 ] ) / s;
			q.z = ( r[ 0 ][ 2 ] + r[ 2 ][ 0 ] ) / s;
		} else if ( r[ 1 ][ 1 ] > r[ 2 ][ 2 ] ) {
			float s = std::sqrt( 1.f + r[ 1 ][ 1 ] - r[ 0 ][ 0 ] - r[ 2 ][ 2 ] ) * 2.f;
			q.w = ( r[ 0 ][ 2 ] - r[ 2 ][ 0 ] ) / s;
			q.x = ( r[ 0 ][ 1 ] + r[ 1 ][ 0 ] ) / s;
			q.y = 0.25f * s;
			q.z = ( r[ 1 ][ 2 ] + r[ 2 ][ 1 ] ) / s;
		} else {
			float s = std::sqrt( 1.f + r[ 2 ][ 2 ] - r[ 0 ][ 0 ] - r[ 1 ][ 1 ] ) * 2.f;
			q.w = ( r[ 1 ][ 0 ] - r[ 0 ][ 1 ] ) / s;
			q.x = ( r[ 0 ][ 2 ] + r[ 2 ][ 0 ] ) / s;
			q.y = ( r[ 1 ][ 2 ] + r[ 2 ][ 1 ] ) / s;
			q.z = 0.25f * s;
		}
		return q;
	}

	// 行列を 移動・回転（四元数）・軸ごとの拡大 に分ける。
	void decompose( const Matrix34& m, float translation[ kAxisNumber ], Quaternion* rotation, float scale[ kAxisNumber ] ) {
		float r[ kAxisNumber ][ kAxisNumber ];
		for ( int column = 0; column < kAxisNumber; ++column ) {
			scale[ column ] = axisLength( m, column );
			float inverse = ( scale[ column ] > kMinScale ) ? 1.f / scale[ column ] : 0.f;
			for ( int row = 0; row < kAxisNumber; ++row ) {
				r[ row ][ column ] = element( m, row, column ) * inverse;
			}
		}
		translation[ 0 ] = m.m03;
		translation[ 1 ] = m.m13;
		translation[ 2 ] = m.m23;
		*rotation = toQuaternion( r );
	}

	// 球面線形補間。回転の角速度が一定になるので、途中で速さが変わらず、軸も縮まない。
	Quaternion slerp( const Quaternion& a, Quaternion b, float t ) {
		float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
		// q と -q は同じ回転。内積が負なら b を裏返して、近い回り方（180度以内）で補間する。
		if ( dot < 0.f ) {
			b.x = -b.x; b.y = -b.y; b.z = -b.z; b.w = -b.w;
			dot = -dot;
		}
		float wa;
		float wb;
		if ( dot > kNlerpThreshold ) {
			wa = 1.f - t;
			wb = t;
		} else {
			float theta = std::acos( dot );
			float sinTheta = std::sin( theta );
			wa = std::sin( ( 1.f - t ) * theta ) / sinTheta;
			wb = std::sin( t * theta ) / sinTheta;
		}
		Quaternion q;
		q.x = a.x * wa + b.x * wb;
		q.y = a.y * wa + b.y * wb;
		q.z = a.z * wa + b.z * wb;
		q.w = a.w * wa + b.w * wb;
		// 近い回転の代用（正規化線形補間）では長さが1からずれるので、正規化する。
		float length = std::sqrt( q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w );
		q.x /= length; q.y /= length; q.z /= length; q.w /= length;
		return q;
	}
}

void MatrixBlend::blend( const Matrix34& from, const Matrix34& to, float t, Matrix34* out ) {
	float fromTranslation[ kAxisNumber ];
	float toTranslation[ kAxisNumber ];
	float fromScale[ kAxisNumber ];
	float toScale[ kAxisNumber ];
	Quaternion fromRotation;
	Quaternion toRotation;
	decompose( from, fromTranslation, &fromRotation, fromScale );
	decompose( to, toTranslation, &toRotation, toScale );

	Quaternion q = slerp( fromRotation, toRotation, t );
	float s[ kAxisNumber ];
	for ( int i = 0; i < kAxisNumber; ++i ) {
		s[ i ] = ScalarMath::lerp( fromScale[ i ], toScale[ i ], t );
	}

	// 回転行列 × 拡大（列ごとに軸の長さを掛ける）＋移動 で組み立て直す。
	float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
	float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
	float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
	out->m00 = ( 1.f - 2.f * ( yy + zz ) ) * s[ 0 ];
	out->m01 = ( 2.f * ( xy - wz ) ) * s[ 1 ];
	out->m02 = ( 2.f * ( xz + wy ) ) * s[ 2 ];
	out->m10 = ( 2.f * ( xy + wz ) ) * s[ 0 ];
	out->m11 = ( 1.f - 2.f * ( xx + zz ) ) * s[ 1 ];
	out->m12 = ( 2.f * ( yz - wx ) ) * s[ 2 ];
	out->m20 = ( 2.f * ( xz - wy ) ) * s[ 0 ];
	out->m21 = ( 2.f * ( yz + wx ) ) * s[ 1 ];
	out->m22 = ( 1.f - 2.f * ( xx + yy ) ) * s[ 2 ];
	out->m03 = ScalarMath::lerp( fromTranslation[ 0 ], toTranslation[ 0 ], t );
	out->m13 = ScalarMath::lerp( fromTranslation[ 1 ], toTranslation[ 1 ], t );
	out->m23 = ScalarMath::lerp( fromTranslation[ 2 ], toTranslation[ 2 ], t );
}
