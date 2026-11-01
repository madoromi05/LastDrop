#include "GameSrc/Interaction/Interactor.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Inventory.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

Interactor::Interactor( float reach, Finder finder ) :
	mReach( reach ),
	mFinder( finder ) {
	ASSERT( mReach > 0.f && mFinder && "Interactor : bad argument." );
}

void Interactor::update( const Vector3& eye, const Vector3& direction,
	Inventory* inventory, bool interactPressed ) {
	ASSERT( inventory && "Interactor : inventory is null." );
	mPrompt.clear();
	Vector3 delta = direction;
	delta *= mReach;
	IInteractable* target = mFinder( eye, delta );
	if ( !target ) {
		return;
	}
	if ( interactPressed && target->canInteract( *inventory ) ) {
		target->interact( inventory );
	}
	// 使った後の案内を出す（拾った直後は「持ちきれない」、給油した直後は新しい残り数になる）。
	mPrompt = target->prompt( *inventory );
}

const std::string& Interactor::prompt() const {
	return mPrompt;
}
