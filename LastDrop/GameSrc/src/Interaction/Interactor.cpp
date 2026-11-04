#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Inventory.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Interactor::Interactor( float reach, Finder finder ) :
	mReach( reach ),
	mFinder( finder ),
	mTarget( nullptr ),
	mOffer( nullptr ) {
	ASSERT( mReach > 0.f && mFinder && "Interactor : bad argument." );
}

void Interactor::update( const Vector3& eye, const Vector3& direction,
	Inventory* inventory, bool interactPressed ) {
	ASSERT( inventory && "Interactor : inventory is null." );
	Vector3 delta = direction;
	delta *= mReach;
	IInteractable* target = mFinder( eye, delta );
	mTarget = target;
	mOffer = nullptr;
	if ( !target || !interactPressed || !target->canInteract( *inventory ) ) {
		return;
	}
	// 拾う物は、置く所を選ぶまで入れない（呼ぶ側がカバンの画面を開く）。それ以外はすぐ使う。
	ItemStack offered;
	if ( target->offeredStack( &offered ) ) {
		mOffer = target;
	} else {
		target->interact( inventory );
	}
}

IInteractable* Interactor::offer() const {
	return mOffer;
}

const IInteractable* Interactor::target() const {
	return mTarget;
}
