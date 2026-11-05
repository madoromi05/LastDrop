#include "GameSrc/Interaction/LootSearch.h"
#include "GameSrc/Interaction/LootContainerComponent.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"

LootSearch::LootSearch() :
	mElapsed( 0.f ),
	mDuration( 0.f ),
	mActive( false ) {
}

void LootSearch::start( const GameObject* target, float seconds ) {
	ASSERT( target && seconds > 0.f && "LootSearch::start : bad argument." );
	mTarget = GameObjectHandle( target );
	mElapsed = 0.f;
	mDuration = seconds;
	mActive = true;
}

void LootSearch::cancel() {
	mActive = false;
	mTarget = GameObjectHandle();
}

bool LootSearch::isSearching() const {
	return mActive;
}

LootSearch::Step LootSearch::tick( float seconds, bool interrupted ) {
	if ( !mActive ) {
		return Step::Idle;
	}
	if ( interrupted || !target() ) {
		cancel();
		return Step::Cancelled;
	}
	mElapsed += seconds;
	if ( mElapsed < mDuration ) {
		return Step::Searching;
	}
	mActive = false; // 場所は container で読めるよう残す（次の start か cancel まで）
	return Step::Finished;
}

LootContainerComponent* LootSearch::container() const {
	GameObject* object = mTarget.get();
	return object ? object->getComponent< LootContainerComponent >() : nullptr;
}

GameObject* LootSearch::target() const {
	return mTarget.get();
}

float LootSearch::progress() const {
	if ( !mActive || mDuration <= 0.f ) {
		return 0.f;
	}
	float ratio = mElapsed / mDuration;
	return ( ratio < 1.f ) ? ratio : 1.f;
}
