#include "SweepMath.h"

#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// これより小さい軸方向の移動は、その軸には動かないとみなす（m）。0 割りを避ける。
	const float kMinMove = 0.000001f;
	const float kStartTime = 0.f;
	const float kEndTime = 1.f;
	const float kHalf = 0.5f;
	const int kAxesPerBox = 2;
	// 調べる軸の数の上限。長方形どうしなら 4 本（2 + 2）、長方形とかけらなら「長方形の 2 本＋かけらの辺の法線」。
	const int kMaxAxisNumber = kAxesPerBox + WallPiece::kMaxCorners;

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

	// 分離軸判定に使う軸（水平・長さ1）と、それぞれの軸へ2つの形を影として落としたときの
	// 「影の半径の和」「相手の影の中心から自分の影の中心への距離」。
	// 形が長方形でも多角形でも、影にしてしまえば同じ計算で解ける（overlapPush / sweepAlongAxes）。
	struct SeparatingAxes{
		Vector3 axis[ kMaxAxisNumber ];
		float radiusSum[ kMaxAxisNumber ];
		float distance[ kMaxAxisNumber ];
		int number;
	};

	// 長方形どうし。軸は、2つの長方形の辺の向き（それぞれ2本、計4本）。
	SeparatingAxes separatingAxes( const BoxCollider& box, const BoxCollider& other ) {
		Vector3 boxAxis[ kAxesPerBox ];
		Vector3 otherAxis[ kAxesPerBox ];
		boxAxes( box, boxAxis );
		boxAxes( other, otherAxis );
		Vector3 offset;
		offset.setSub( box.center, other.center );
		offset = flatten( offset );
		SeparatingAxes result;
		result.number = kAxesPerBox * 2;
		for ( int i = 0; i < kAxesPerBox; ++i ) {
			result.axis[ i ] = boxAxis[ i ];
			result.axis[ kAxesPerBox + i ] = otherAxis[ i ];
		}
		for ( int i = 0; i < result.number; ++i ) {
			result.radiusSum[ i ] = projectedRadius( box, boxAxis, result.axis[ i ] ) + projectedRadius( other, otherAxis, result.axis[ i ] );
			result.distance[ i ] = offset.dot( result.axis[ i ] );
		}
		return result;
	}

	// 長方形と、面のかけら（真上から見た凸な多角形）。軸は、長方形の辺の向き2本と、かけらの辺の法線。
	// かけらの影は、隅を軸へ落とした一番小さい値〜一番大きい値。その真ん中を影の中心、幅の半分を影の半径とする。
	SeparatingAxes separatingAxes( const BoxCollider& box, const WallPiece& piece ) {
		Vector3 boxAxis[ kAxesPerBox ];
		boxAxes( box, boxAxis );
		Vector3 boxCenter = flatten( box.center );
		SeparatingAxes result;
		result.number = kAxesPerBox + piece.edgeNormalNumber;
		for ( int i = 0; i < kAxesPerBox; ++i ) {
			result.axis[ i ] = boxAxis[ i ];
		}
		for ( int i = 0; i < piece.edgeNormalNumber; ++i ) {
			result.axis[ kAxesPerBox + i ] = piece.edgeNormal[ i ];
		}
		for ( int i = 0; i < result.number; ++i ) {
			const Vector3& axis = result.axis[ i ]; // y は 0 なので、隅の y は影に効かない
			float low = FLOAT_MAX;
			float high = -FLOAT_MAX;
			for ( int j = 0; j < piece.cornerNumber; ++j ) {
				float shadow = piece.corner[ j ].dot( axis );
				low = Math::min( low, shadow );
				high = Math::max( high, shadow );
			}
			result.radiusSum[ i ] = projectedRadius( box, boxAxis, axis ) + ( high - low ) * kHalf;
			result.distance[ i ] = boxCenter.dot( axis ) - ( low + high ) * kHalf;
		}
		return result;
	}

	// 重なり：どの軸に影を落としても重なっていれば重なっている。一番浅い軸の向きへ押し出すのが最短。
	// 重なっていれば、外へ出す最短の移動量を push に書いて true。ちょうど接しているだけなら false。
	bool overlapPush( const SeparatingAxes& axes, Vector3* push ) {
		float shallowest = FLOAT_MAX;
		Vector3 pushDirection( 0.f, 0.f, 0.f );
		for ( int i = 0; i < axes.number; ++i ) {
			float overlap = axes.radiusSum[ i ] - Math::abs( axes.distance[ i ] );
			if ( overlap <= 0.f ) {
				return false; // この軸で影が離れている（接しているだけも含む）＝重なっていない
			}
			if ( overlap < shallowest ) {
				shallowest = overlap;
				pushDirection.setMul( axes.axis[ i ], signOf( axes.distance[ i ] ) );
			}
		}
		push->setMul( pushDirection, shallowest );
		return true;
	}

	// 動き始めから重なっているときの当たり。奥へ進もうとしていれば time=0 で当たり、離れる向きなら当たらない。
	// push: 外へ出す向き（overlapPush の結果。長さ0でないこと）
	bool hitWhileOverlapping( const Vector3& push, const Vector3& delta, SweepHit* hit ) {
		if ( push.dot( delta ) >= 0.f ) {
			return false;
		}
		hit->time = kStartTime;
		hit->normal = push;
		hit->normal.normalize();
		return true;
	}

	// 掃引：軸ごとに「影が重なり始める時刻・離れる時刻」を求める。全部の軸で重なっている一番早い時刻が当たった時刻で、
	// 最後に重なり始めた軸がぶつかった面の向きになる。move: 自分の移動量（水平）。
	bool sweepAlongAxes( const SeparatingAxes& axes, const Vector3& move, SweepHit* hit ) {
		Vector3 push;
		if ( overlapPush( axes, &push ) ) {
			return hitWhileOverlapping( push, move, hit );
		}
		float enterTime = -FLOAT_MAX;
		float exitTime = FLOAT_MAX;
		Vector3 enterNormal( 0.f, 0.f, 0.f );
		for ( int i = 0; i < axes.number; ++i ) {
			const Vector3& axis = axes.axis[ i ];
			float distance = axes.distance[ i ];
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
}

bool SweepMath::sweepBox( const BoxCollider& box, const Vector3& delta, const BoxCollider& other, SweepHit* hit ) {
	return sweepAlongAxes( separatingAxes( box, other ), flatten( delta ), hit );
}

bool SweepMath::penetrateBox( const BoxCollider& box, const BoxCollider& other, Vector3* push ) {
	return overlapPush( separatingAxes( box, other ), push );
}

bool SweepMath::sweepPiece( const BoxCollider& box, const Vector3& delta, const WallPiece& piece, SweepHit* hit ) {
	return sweepAlongAxes( separatingAxes( box, piece ), flatten( delta ), hit );
}

bool SweepMath::penetratePiece( const BoxCollider& box, const WallPiece& piece, Vector3* push ) {
	return overlapPush( separatingAxes( box, piece ), push );
}
