#include "GameSrc/Interaction/Pickup.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Hit/HitWorld.h"
#include "GameSrc/Collision/CollisionWorld.h"
#include "GameSrc/Hit/HitMath.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"

#include <string>

using namespace GameLib::Math;

namespace{
	// 落ちている物（ガソリンの携行缶・ゾンビが落とした弾）はどれも金属。木箱などを置くときは、作るときに受け取る形に変える。
	const GroundMaterial kBodyMaterial = GROUND_METAL;
	// 見る箱を、頂点を囲む箱から四方と上へ広げる長さ（m）。2026-10-31 ユーザー承認（7.62 の弾薬を狙わずに拾えるように）。
	const float kLookMargin = 0.15f;
	const float kHalf = 0.5f;

	// 見る箱（box を kLookMargin だけ広げ、地面 Y=0 から上へ立てた箱）を -1〜1 の立方体にする行列。
	Matrix34 makeLookBoxToLocal( const BoxCollider& box ) {
		float halfHeight = box.height * kHalf + kLookMargin;
		Matrix34 world = PlacementMatrix::make( Vector3( box.center.x, box.height * kHalf, box.center.z ), box.angleY );
		world.scale( box.halfWidthX + kLookMargin, halfHeight, box.halfWidthZ + kLookMargin );
		Matrix34 toLocal;
		toLocal.setInverse( world );
		return toLocal;
	}
}

Pickup::Pickup( const StaticMeshData* data, const Vector3& position, float angleY, ItemId item, int amount, bool blocksMovement ) :
	mBody( data, position, angleY, kBodyMaterial ),
	mItem( item ),
	mAmount( amount ),
	mBlocksMovement( blocksMovement ),
	mTaken( false ) {
	ASSERT( amount > 0 && "Pickup : amount must be positive." );
	mLookBoxToLocal = makeLookBoxToLocal( mBody.collider() ); // mBody を先に置いてから作る（メンバの宣言順）
}

void Pickup::draw( const DrawCulling& culling ) const {
	mBody.draw( culling, DrawCulling::Target::Small );
}

bool Pickup::isTaken() const {
	return mTaken;
}

bool Pickup::lookRaycast( const Vector3& origin, const Vector3& delta, float* time ) const {
	ASSERT( time && "Pickup::lookRaycast : time is null." );
	if ( mTaken ) {
		return false;
	}
	Vector3 localOrigin;
	Vector3 localDelta;
	mLookBoxToLocal.mul( &localOrigin, origin );
	mLookBoxToLocal.mul33( &localDelta, delta );
	Vector3 localNormal; // 向きは使わない
	return HitMath::segmentUnitCube( localOrigin, localDelta, time, &localNormal );
}

bool Pickup::raycast( const Vector3& origin, const Vector3& delta, float* time, Vector3* normal ) const {
	return mBody.raycast( origin, delta, time, normal );
}

GroundMaterial Pickup::surfaceMaterial( const Vector3& point ) const {
	return mBody.surfaceMaterial( point );
}

bool Pickup::canInteract( const Inventory& inventory ) const {
	return !mTaken && inventory.canAdd( mItem );
}

std::string Pickup::prompt( const Inventory& inventory ) const {
	const std::string name = itemSpec( mItem ).name;
	if ( !inventory.canAdd( mItem ) ) {
		return "Can't carry more " + name;
	}
	// 1 つだけなら数は書かない（ガソリンの「Pick up Fuel」のまま）。
	if ( mAmount > 1 ) {
		return "[E] Pick up " + name + " (" + std::to_string( mAmount ) + ")";
	}
	return "[E] Pick up " + name;
}

void Pickup::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "Pickup::interact : cannot pick up." );
	while ( mAmount > 0 && inventory->canAdd( mItem ) ) {
		inventory->add( mItem );
		--mAmount;
	}
	mTaken = ( mAmount == 0 );
}

BoxCollider Pickup::collider() const {
	return mBody.collider();
}

void Pickup::addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	ASSERT( collisionWorld && hitWorld && "Pickup : world is null." );
	if ( mBlocksMovement ) {
		collisionWorld->addBody( this );
	}
	hitWorld->addBlocker( this );
}

void Pickup::removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) {
	ASSERT( collisionWorld && hitWorld && "Pickup : world is null." );
	if ( mBlocksMovement ) {
		collisionWorld->removeBody( this );
	}
	hitWorld->removeBlocker( this );
}
