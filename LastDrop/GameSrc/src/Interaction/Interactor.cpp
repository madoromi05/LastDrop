#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Inventory.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Interactor::Interactor( float reach, Finder finder ) :
	mReach( reach ),
	mFinder( finder ),
	mTarget( nullptr ) {
	ASSERT( mReach > 0.f && mFinder && "Interactor : bad argument." );
}

void Interactor::update( const Vector3& eye, const Vector3& direction,
	Inventory* inventory, bool interactPressed ) {
	ASSERT( inventory && "Interactor : inventory is null." );
	Vector3 delta = direction;
	delta *= mReach;
	IInteractable* target = mFinder( eye, delta );
	mTarget = target;
	if ( target && interactPressed && target->canInteract( *inventory ) ) {
		target->interact( inventory );
	}
}

const IInteractable* Interactor::target() const {
	return mTarget;
}
