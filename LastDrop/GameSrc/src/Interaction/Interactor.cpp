#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Inventory.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Interactor::Interactor( float reach, Finder finder ) :
	mReach( reach ),
	mFinder( finder ),
	mTarget( nullptr ),
	mOffer( nullptr ),
	mOpened( nullptr ),
	mTimedUse( nullptr ) {
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
	mTimedUse = nullptr;
	if ( !target || !interactPressed || !target->canInteract( *inventory ) ) {
		return;
	}
	// 漁れる場所は、開けるまで待ってから中を見せる（呼ぶ側）。拾う物は、置く所を選ぶまで入れない（呼ぶ側がカバンの画面を開く）。
	// 時間をかけて使う物（給油）は、待ってから使う（呼ぶ側）。それ以外はすぐ使う。
	ItemStack offered;
	if ( target->lootGrid() ) {
		mOpened = target;
	} else if ( target->useSeconds() > 0.f ) {
		mTimedUse = target;
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

IInteractable* Interactor::timedUse() const {
	return mTimedUse;
}

const IInteractable* Interactor::target() const {
	return mTarget;
}
