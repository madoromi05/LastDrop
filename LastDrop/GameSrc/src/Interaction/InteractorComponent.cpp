#include "GameSrc/Interaction/InteractorComponent.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/IInteractFinder.h"
#include "GameSrc/Interaction/Item/InventoryComponent.h"
#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/World/GameObjectWorld.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

InteractorComponent::InteractorComponent( float reach ) :
	mReach( reach ),
	mFinder( nullptr ),
	mInventory( nullptr ),
	mTarget( nullptr ),
	mOffer( nullptr ),
	mOpened( nullptr ) {
	ASSERT( mReach > 0.f && "InteractorComponent : reach must be positive." );
}

void InteractorComponent::start() {
	mInventory = requireComponent< InventoryComponent >();
	mFinder = shared< IInteractFinder >();
}

void InteractorComponent::look( const Vector3& eye, const Vector3& direction, bool interactPressed ) {
	Vector3 delta = direction;
	delta *= mReach;
	const IHittable* viewer = getComponent< HitboxComponent >(); // 自分の体には視線を当てない（目が体の中にあるため）。外せる部品（無ければ除く的が無い）
	IInteractable* target = mFinder->findInteractable( eye, delta, viewer );
	mTarget = target;
	mOffer = nullptr;
	mOpened = nullptr;
	Inventory& inventory = mInventory->inventory();
	if ( !target || !interactPressed || !target->canInteract( inventory ) ) {
		return;
	}
	// 漁れる場所・脱出の車の給油口は、中のマス目を見せる（開けるまで待つ・画面を開くのは呼ぶ側）
	// 拾う物は、置く所を選ぶまで入れない（呼ぶ側がカバンの画面を開く）。それ以外はすぐ使う
	ItemStack offered;
	if ( target->lootGrid() ) {
		mOpened = target;
	} else if ( target->offeredStack( &offered ) ) {
		mOffer = target;
	} else {
		target->interact( &inventory );
	}
}

IInteractable* InteractorComponent::offer() const {
	return mOffer;
}

IInteractable* InteractorComponent::opened() const {
	return mOpened;
}

const IInteractable* InteractorComponent::target() const {
	return mTarget;
}
