#include "GameSrc/Collision/CharacterMover.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Collision/ICollisionBody.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// 当たった面の手前に残す隙間（m）。面の「ちょうど上」で止めると、浮動小数の誤差で次の掃引が面の内側から始まり、
	// 壁に引っかかって滑らなくなるため。Unity の skinWidth と同じ役割。歩く速さ（約2cm/フレーム）より十分小さくする。
	const float kSkinWidth = 0.001f;
	// 滑り直す回数の上限。壁に当たって滑る → 次の壁に当たって滑る、の連鎖を何回まで追うか。
	// 2D で角に挟まれても 2 回で止まるので、3 回あれば足りる。
	const int kMaxSlideIterations = 3;
	// めり込みを外へ出し直す回数の上限。押し出した先で別の相手にめり込んでいる（群れに囲まれた）ときに繰り返す。
	const int kMaxDepenetrationIterations = 3;
	// これより短い移動は進まない（m）。滑らせた残りが誤差だけになったら打ち切る。
	const float kMinMoveDistance = 0.0001f;
	// 足元を地面の高さへ近づける速さ（m/ティック。秒あたりで決めて直す）。
	// 縁石（15cm）を約 0.1 秒で上り下りする。速すぎると視点が跳ね、遅すぎると歩道の上でしばらく足が沈んで見える。
	const float kGroundFollowSpeed = 1.5f / GameTime::kTicksPerSecond;
}

CharacterMover::CharacterMover( const ICollisionBody* self ) :
	mWorld( nullptr ),
	mSelf( self ) {
	ASSERT( mSelf && "CharacterMover : self is null." );
}

void CharacterMover::setWorld( const CollisionWorld* world ) {
	ASSERT( world && !mWorld && "CharacterMover : world is null or already set." );
	mWorld = world;
}

Vector3 CharacterMover::move( const Vector3& delta ) const {
	return move( delta, CollisionWorld::BodyFilter{ mSelf, nullptr, false } );
}

Vector3 CharacterMover::movePassingThrough( const Vector3& delta, const ICollisionBody* passThrough ) const {
	return move( delta, CollisionWorld::BodyFilter{ mSelf, passThrough, false } );
}

Vector3 CharacterMover::moveIgnoringBodies( const Vector3& delta ) const {
	return move( delta, CollisionWorld::BodyFilter{ mSelf, nullptr, true } );
}

Vector3 CharacterMover::move( const Vector3& delta, const CollisionWorld::BodyFilter& filter ) const {
	ASSERT( mWorld && "CharacterMover : world is not set." );
	BoxCollider box = mSelf->collider();
	Vector3 start = box.center;
	depenetrate( &box, filter );

	Vector3 remaining( delta.x, 0.f, delta.z );
	Vector3 previousNormal( 0.f, 0.f, 0.f );
	bool hasPreviousNormal = false;
	for ( int i = 0; i < kMaxSlideIterations; ++i ) {
		float length = remaining.length();
		if ( length < kMinMoveDistance ) {
			break;
		}
		SweepHit hit;
		if ( !mWorld->sweep( box, remaining, filter, &hit ) ) {
			box.center += remaining;
			break;
		}
		// 当たる所の少し手前まで進む。
		float travel = Math::max( 0.f, hit.time * length - kSkinWidth );
		float travelRate = travel / length;
		box.center.madd( remaining, travelRate );
		remaining *= ( 1.f - travelRate );
		// 残りから面へ向かう成分を除き、面に沿って滑らせる。
		remaining.madd( hit.normal, -remaining.dot( hit.normal ) );
		// 入り隅（2枚の壁が作る角）では、今の面に沿う向きが前の面へ入り込む向きになる。
		// 2D ではどちらの面にも沿える向きが無いので、そこで止まる（行ったり来たりの振動を防ぐ）。
		if ( hasPreviousNormal && remaining.dot( previousNormal ) < 0.f ) {
			break;
		}
		previousNormal = hit.normal;
		hasPreviousNormal = true;
	}
	// 長方形は回さず平行に動かしただけなので、中心の動きがそのまま足元の動きになる。
	Vector3 moved;
	moved.setSub( box.center, start );
	return moved;
}

float CharacterMover::followGround( const Vector3& position ) const {
	ASSERT( mWorld && "CharacterMover : world is not set." );
	float ground;
	if ( !mWorld->groundHeight( position, &ground ) ) {
		return position.y;
	}
	float step = Math::max( -kGroundFollowSpeed, Math::min( ground - position.y, kGroundFollowSpeed ) );
	return position.y + step;
}

void CharacterMover::depenetrate( BoxCollider* box, const CollisionWorld::BodyFilter& filter ) const {
	for ( int i = 0; i < kMaxDepenetrationIterations; ++i ) {
		Vector3 push;
		if ( !mWorld->findPenetration( *box, filter, &push ) ) {
			break;
		}
		box->center += push;
	}
}
