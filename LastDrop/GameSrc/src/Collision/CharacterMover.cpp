#include "GameSrc/Collision/CharacterMover.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Collision/ICollisionBody.h"

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
}

CharacterMover::CharacterMover( const CollisionWorld* world, const ICollisionBody* self ) :
	mWorld( world ),
	mSelf( self ) {
	ASSERT( mWorld && "CharacterMover : world is null." );
	ASSERT( mSelf && "CharacterMover : self is null." );
}

Vector3 CharacterMover::move( const Vector3& delta ) const {
	BoxCollider box = mSelf->collider();
	Vector3 start = box.center;
	depenetrate( &box );

	Vector3 remaining( delta.x, 0.f, delta.z );
	Vector3 previousNormal( 0.f, 0.f, 0.f );
	bool hasPreviousNormal = false;
	for ( int i = 0; i < kMaxSlideIterations; ++i ) {
		float length = remaining.length();
		if ( length < kMinMoveDistance ) {
			break;
		}
		SweepHit hit;
		if ( !mWorld->sweep( box, remaining, mSelf, &hit ) ) {
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

void CharacterMover::depenetrate( BoxCollider* box ) const {
	for ( int i = 0; i < kMaxDepenetrationIterations; ++i ) {
		Vector3 push;
		if ( !mWorld->findPenetration( *box, mSelf, &push ) ) {
			break;
		}
		box->center += push;
	}
}
