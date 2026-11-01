#include "GameSrc/Interaction/PickupComponent.h"
#include "GameSrc/Interaction/ItemComponent.h"
#include "GameSrc/Interaction/Inventory.h"
#include "GameSrc/Component/StaticModelComponent.h"
#include "GameSrc/Collision/MeshBoundsBox.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Hit/HitMath.h"
#include "GameSrc/Math/PlacementMatrix.h"

#include "GameLib/GameLib.h"

#include <string>

using namespace GameLib::Math;

namespace{
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

PickupComponent::PickupComponent() :
	mItem( nullptr ),
	mTaken( false ) {
}

void PickupComponent::start() {
	mItem = requireComponent< ItemComponent >();
	// 頂点を囲む箱は、見た目から求める（BoxColliderComponent と同じ箱。見た目は awake で置き場所へ動いている）。
	mLookBoxToLocal = makeLookBoxToLocal( MeshBoundsBox::make( requireComponent< StaticModelComponent >()->mesh() ) );
}

bool PickupComponent::isTaken() const {
	return mTaken;
}

bool PickupComponent::lookRaycast( const Vector3& origin, const Vector3& delta, float* time ) const {
	ASSERT( time && "PickupComponent::lookRaycast : time is null." );
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

bool PickupComponent::canInteract( const Inventory& inventory ) const {
	return !mTaken && inventory.canAdd( mItem->item() );
}

std::string PickupComponent::prompt( const Inventory& inventory ) const {
	const std::string name = itemSpec( mItem->item() ).name;
	if ( !inventory.canAdd( mItem->item() ) ) {
		return "Can't carry more " + name;
	}
	// 1 つだけなら数は書かない（ガソリンの「Pick up Fuel」のまま）。
	if ( mItem->amount() > 1 ) {
		return "[E] Pick up " + name + " (" + std::to_string( mItem->amount() ) + ")";
	}
	return "[E] Pick up " + name;
}

void PickupComponent::interact( Inventory* inventory ) {
	ASSERT( inventory && canInteract( *inventory ) && "PickupComponent::interact : cannot pick up." );
	while ( mItem->amount() > 0 && inventory->canAdd( mItem->item() ) ) {
		inventory->add( mItem->item() );
		mItem->takeOne();
	}
	mTaken = ( mItem->amount() == 0 );
}
