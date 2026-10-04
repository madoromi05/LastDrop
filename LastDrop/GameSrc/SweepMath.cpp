#include "SweepMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// これより小さい軸方向の移動は、その軸には動かないとみなす（m）。0 割りを避ける。
	const float kMinMove = 0.000001f;
	const float kStartTime = 0.f;
	const float kEndTime = 1.f;
	const int kAxesPerBox = 2;
	const int kAxisNumber = kAxesPerBox * 2; // 2つの長方形の辺の向き

	// 水平成分だけにする（y を 0 に）。
	Vector3 flatten( const Vector3& v ) {
		return Vector3( v.x, 0.f, v.z );
	}

	// 0 は正として扱う符号（押し出す向きを必ずどちらかに決めるため）。
	float signOf( float value ) {
		return ( value < 0.f ) ? -1.f : 1.f;
	}

	// 長方形のローカルの X 軸・Z 軸がワールドで向く向き（長さ1）。Matrix34::rotateY と同じ回転の向き。
	void boxAxes( const BoxCollider& box, Vector3 axes[ kAxesPerBox ] ) {
		float c = Math::cos( box.angleY );
		float s = Math::sin( box.angleY );
		axes[ 0 ].set( c, 0.f, -s );
		axes[ 1 ].set( s, 0.f, c );
	}

	// 長方形を軸 axis（長さ1）へ影として落としたときの、中心から端までの長さ。
	float projectedRadius( const BoxCollider& box, const Vector3 axes[ kAxesPerBox ], const Vector3& axis ) {
		return box.halfWidthX * Math::abs( axes[ 0 ].dot( axis ) ) + box.halfWidthZ * Math::abs( axes[ 1 ].dot( axis ) );
	}

	// 分離軸判定に使う、2つの長方形の辺の向き4本と、それぞれの軸での「2つの影の半径の和」。
	struct SeparatingAxes{
		Vector3 axis[ kAxisNumber ];
		float radiusSum[ kAxisNumber ];
	};
	SeparatingAxes separatingAxes( const BoxCollider& box, const BoxCollider& other ) {
		Vector3 boxAxis[ kAxesPerBox ];
		Vector3 otherAxis[ kAxesPerBox ];
		boxAxes( box, boxAxis );
		boxAxes( other, otherAxis );
		SeparatingAxes result;
		for ( int i = 0; i < kAxesPerBox; ++i ) {
			result.axis[ i ] = boxAxis[ i ];
			result.axis[ kAxesPerBox + i ] = otherAxis[ i ];
		}
		for ( int i = 0; i < kAxisNumber; ++i ) {
			result.radiusSum[ i ] = projectedRadius( box, boxAxis, result.axis[ i ] ) + projectedRadius( other, otherAxis, result.axis[ i ] );
		}
		return result;
	}

	// 動き始めから重なっているときの当たり。奥へ進もうとしていれば time=0 で当たり、離れる向きなら当たらない。
	// push: 外へ出す向き（penetrateBox の結果。長さ0でないこと）
	bool hitWhileOverlapping( const Vector3& push, const Vector3& delta, SweepHit* hit ) {
		if ( push.dot( delta ) >= 0.f ) {
			return false;
		}
		hit->time = kStartTime;
		hit->normal = push;
		hit->normal.normalize();
		return true;
	}
}

bool SweepMath::sweepBox( const BoxCollider& box, const Vector3& delta, const BoxCollider& other, SweepHit* hit ) {
	Vector3 move = flatten( delta );
	Vector3 push;
	if ( penetrateBox( box, other, &push ) ) {
		return hitWhileOverlapping( push, move, hit );
	}
	Vector3 offset;
	offset.setSub( box.center, other.center );
	offset = flatten( offset );
	SeparatingAxes axes = separatingAxes( box, other );

	float enterTime = -FLOAT_MAX;
	float exitTime = FLOAT_MAX;
	Vector3 enterNormal( 0.f, 0.f, 0.f );
	for ( int i = 0; i < kAxisNumber; ++i ) {
		const Vector3& axis = axes.axis[ i ];
		float distance = offset.dot( axis ); // この軸での、相手から自分への中心の距離
		float speed = move.dot( axis );
		float radiusSum = axes.radiusSum[ i ];
		if ( Math::abs( speed ) < kMinMove ) {
			// この軸には動かない。影が離れていれば決して当たらない。
			if ( Math::abs( distance ) >= radiusSum ) {
				return false;
			}
			continue;
		}
		float toLow = ( -radiusSum - distance ) / speed;
		float toHigh = ( radiusSum - distance ) / speed;
		float axisEnter = Math::min( toLow, toHigh );
		if ( axisEnter > enterTime ) {
			enterTime = axisEnter;
			enterNormal.setMul( axis, signOf( distance ) ); // ぶつかる直前に自分がいた側
		}
		exitTime = Math::min( exitTime, Math::max( toLow, toHigh ) );
		if ( enterTime > exitTime ) {
			return false;
		}
	}
	if ( enterTime < kStartTime || enterTime > kEndTime ) {
		return false;
	}
	hit->time = enterTime;
	hit->normal = enterNormal;
	return true;
}

bool SweepMath::penetrateBox( const BoxCollider& box, const BoxCollider& other, Vector3* push ) {
	Vector3 offset;
	offset.setSub( box.center, other.center );
	offset = flatten( offset );
	SeparatingAxes axes = separatingAxes( box, other );

	float shallowest = FLOAT_MAX;
	Vector3 pushDirection( 0.f, 0.f, 0.f );
	for ( int i = 0; i < kAxisNumber; ++i ) {
		float distance = offset.dot( axes.axis[ i ] );
		float overlap = axes.radiusSum[ i ] - Math::abs( distance );
		if ( overlap <= 0.f ) {
			return false; // この軸で影が離れている（接しているだけも含む）＝重なっていない
		}
		if ( overlap < shallowest ) {
			shallowest = overlap;
			pushDirection.setMul( axes.axis[ i ], signOf( distance ) );
		}
	}
	push->setMul( pushDirection, shallowest );
	return true;
}
