#include "GameSrc/Scene/Game/Screen/BagController.h"
#include "GameSrc/Scene/Game/Screen/ModalController.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Interaction/Item/ItemCatalog.h"
#include "GameSrc/Interaction/Item/MagazineSync.h"
#include "GameSrc/Interaction/IInteractable.h"
#include "GameSrc/Interaction/Loot/LootParams.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Player/Camera/FirstPersonControl.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Player/PlayerHealthComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Weapon/WeaponCatalog.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Core/Pad.h"
#include "GameLib/GameLib.h"

BagController::BagController( Inventory* inventory, GameWorld* world, FirstPersonControl* control, ModalController* modals, UiRenderer* ui ) :
	mInventory( inventory ),
	mWorld( world ),
	mControl( control ),
	mModals( modals ),
	mScreen( ui ),
	mOffer( nullptr ) {
}

bool BagController::canOpen() const {
	return mWorld->player() && !mWorld->isPlayerDead();
}

void BagController::open() {
	// 銃のマスの「弾倉の数/大きさ」を今の数にする（町の中では銃の物が本当の数を持つ）
	storeMagazineRounds( *mInventory, *mWorld );
	mScreen.open();
	mModals->enter( ModalController::Mode::Bag ); // 一時停止と同じ遷移(音を止め、入力を放し、カーソルを外す)
}

void BagController::close() {
	mOffer = nullptr; // 入れていない分は地面に残る
	mModals->exit();
}

void BagController::startPlacing( IInteractable* offer ) {
	ItemStack stack;
	if ( !canOpen() || !offer->offeredStack( &stack ) ) {
		return;
	}
	open();
	mOffer = offer;
	ItemStack shown;
	offeredStack( &shown );
	mScreen.openForPlacing( *mInventory, shown );
}

bool BagController::offeredStack( ItemStack* out ) const {
	if ( !mOffer || !mOffer->offeredStack( out ) ) {
		return false;
	}
	// 銃は弾倉の数を銃の物から読む（拾った銃のマスにも「30/30」が出るように）
	if ( weaponForItem( out->item ) != WEAPON_NONE ) {
		out->loadedRounds = magazineRoundsOf( mWorld->objects(), out->item );
	}
	return true;
}

void BagController::placeOffered( int cell ) {
	ItemStack stack;
	if ( !offeredStack( &stack ) ) {
		return;
	}
	int count = mInventory->countPlaceableInto( cell, stack );
	if ( count <= 0 ) {
		return;
	}
	mInventory->placeInto( cell, stack, count );
	mOffer->takeOffered( count ); // 入りきらない分は持ったまま（続けて別のマスへ置ける）
	finishPlacingIfDone();
}

void BagController::quickPlaceOffered() {
	if ( !mOffer->canInteract( *mInventory ) ) {
		return; // どこにも入らない（捨てる・並べ替えるで空けてから）
	}
	mOffer->interact( mInventory );              // 今までの自動の入れ方（重ねられるマス → 上の行・左の列の空き）
	storeMagazineRounds( *mInventory, *mWorld ); // 拾った銃のマスにも弾倉の数を出す
	finishPlacingIfDone();
}

void BagController::finishPlacingIfDone() {
	ItemStack rest;
	if ( !offeredStack( &rest ) ) {
		close(); // 全部入れた。拾う操作が長くならないよう、町へ戻す
	}
}

void BagController::handleInput() {
	// Tab でも ESC でも閉じる（ESC で一時停止のメニューは開かない）
	// ESC は拾った物を持っている・メニュー・動かしている途中なら、まずそれを取りやめる（拾うのをやめても開いたまま）
	if ( InputInventory() ) {
		close();
		return;
	}
	if ( InputMenu() ) {
		if ( !mScreen.back() ) {
			close();
		} else if ( !mScreen.isPlacing() ) {
			mOffer = nullptr; // 拾うのをやめた（地面に残る）
		}
		return;
	}
	ItemStack incoming;
	bool placing = mScreen.isPlacing() && offeredStack( &incoming );
	InventoryScreen::Request request = mScreen.handleInput( mInventory, equippedWeapon(), isPlayerInjured(),
		placing ? &incoming : nullptr );
	switch ( request.kind ) {
	case InventoryScreen::Request::USE:
		useFromBag( request.cell );
		break;
	case InventoryScreen::Request::PLACE:
		placeOffered( request.cell );
		break;
	case InventoryScreen::Request::QUICK_PLACE:
		quickPlaceOffered();
		break;
	case InventoryScreen::Request::DROP:
		// 手に持っている銃を捨てたら、閉じた後の最初のティックで素手になる（HoldComponent::syncWithInventory）
		dropFromBag( request.cell );
		break;
	case InventoryScreen::Request::EQUIP: {
		// 開いている間は世界が止まっているので、持ち替えの時間は作らない（RE:2 と同じ）
		GameObject* player = mWorld->player();
		HoldComponent* hold = player ? player->getComponent< HoldComponent >() : nullptr;
		if ( hold ) {
			hold->equip( mInventory->grid().stackAt( request.cell ).item );
		}
		break;
	}
	default:
		break;
	}
}

void BagController::dropFromBag( int cell ) {
	ItemStack dropped = mInventory->takeAll( cell );
	if ( !mWorld->dropItem( dropped, mControl->camera().yaw() ) ) {
		// 置けなかった（形の構築に失敗した）。無くさないようカバンへ戻す（空いたばかりなので必ず入る）
		for ( int i = 0; i < dropped.amount; ++i ) {
			mInventory->add( dropped.item );
		}
	}
}

void BagController::useFromBag( int cell ) {
	GameObject* player = mWorld->player();
	PlayerHealthComponent* health = player ? player->getComponent< PlayerHealthComponent >() : nullptr; // 外せる部品
	if ( !health || !health->health().isInjured() ) {
		return; // 満タンでは使わない（メニューにも出していない）
	}
	ItemId item = mInventory->grid().stackAt( cell ).item;
	ASSERT( itemSpec( item ).kind == ITEM_KIND_MEDICAL && "BagController::useFromBag : not a medical item." );
	health->heal( lootParams().bandageHeal );
	mInventory->remove( item ); // 同じ物なら、どのマスから減っても同じ（数が一番少ないマスから減る）
}

bool BagController::isPlayerInjured() const {
	const GameObject* player = mWorld->player();
	const PlayerHealthComponent* health = player ? player->getComponent< PlayerHealthComponent >() : nullptr;
	return health && health->health().isInjured();
}

PlayerCondition BagController::playerCondition() const {
	const GameObject* player = mWorld->player();
	const PlayerHealthComponent* health = player ? player->getComponent< PlayerHealthComponent >() : nullptr;
	return health ? health->health().condition() : PlayerCondition::Fine;
}

ItemId BagController::equippedWeapon() const {
	const GunComponent* gun = HoldComponent::heldGun( mWorld->player() );
	return gun ? gun->spec().item : ITEM_NONE;
}

void BagController::draw( const char* exits ) {
	ItemStack incoming;
	bool placing = mScreen.isPlacing() && offeredStack( &incoming );
	mScreen.draw( *mInventory, equippedWeapon(), exits, placing ? &incoming : nullptr, playerCondition() ); // HUD は描かない
}
