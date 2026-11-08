#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Item/Inventory.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Interactor::Interactor( float reach, Finder finder ) :
	mReach( reach ),
	mFinder( finder ),
	mTarget( nullptr ),
	mOffer( nullptr ),
	mOpened( nullptr ) {
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
	mOpened = nullptr;
	if ( !target || !interactPressed || !target->canInteract( *inventory ) ) {
		return;
	}
	// 漁れる場所・脱出の車の給油口は、中のマス目を見せる（開けるまで待つ・画面を開くのは呼ぶ側）。
	// 拾う物は、置く所を選ぶまで入れない（呼ぶ側がカバンの画面を開く）。それ以外はすぐ使う。
	ItemStack offered;
	if ( target->lootGrid() ) {
		mOpened = target;
	} else if ( target->offeredStack( &offered ) ) {
		mOffer = target;
	} else {
		target->interact( inventory );
	}
}

IInteractable* Interactor::offer() const {
	return mOffer;
}

IInteractable* Interactor::opened() const {
	return mOpened;
}

const IInteractable* Interactor::target() const {
	return mTarget;
}
