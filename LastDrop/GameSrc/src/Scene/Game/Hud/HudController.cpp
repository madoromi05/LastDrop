#include "GameSrc/Scene/Game/Hud/HudController.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Interaction/Item/InventoryComponent.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameLib/Math/Vector3.h"

HudController::HudController( GameWorld* world, UiRenderer* ui ) :
	mWorld( world ),
	mUi( ui ),
	mHud( ui ),
	mAmmoHud( ui ) {
}

void HudController::updateAmmoVisibility( bool wanted ) {
	mAmmoHudVisibility.update( wanted );
}

void HudController::updateMarkers( const GameLib::Math::Vector3& eye, const IInteractable* target ) {
	const Inventory* bag = inventory();
	if ( !bag ) {
		mInteractMarkers.clear(); // 自キャラが居ない（使える物の色を決める持ち物が無い）
		return;
	}
	mInteractMarkers.update( mWorld->interaction(), eye, target, *bag );
}

const Inventory* HudController::inventory() const {
	return InventoryComponent::inventoryOf( mWorld->player() );
}

void HudController::clearMarkers() {
	mInteractMarkers.clear();
}

void HudController::addAmmo() {
	const GunComponent* gun = HoldComponent::heldGun( mWorld->player() );
	// 消えていく途中に銃を手放した（捨てた・死んだ）ら、その場で出すのをやめる（出す物が無い）
	if ( !gun || !mAmmoHudVisibility.isVisible() ) {
		return;
	}
	AmmoHud::Ammo ammo;
	ammo.weapon = gun->spec().item;
	ammo.rounds = gun->magazineRounds();
	ammo.capacity = gun->spec().magazineCapacity;
	const Inventory* bag = inventory();
	ammo.reserve = bag ? bag->count( gun->spec().ammo ) : 0;
	mAmmoHud.add( ammo, mAmmoHudVisibility.opacity() );
}

void HudController::draw( const FirstPersonCamera& camera, const RaidSession& session, const DayConditionEffect& dayCondition,
	float searchProgress, RaidHud::ProgressKind waitingKind ) {
	// 積むだけ（1 回の描画で UiRenderer::draw がまとめて描く）
	mInteractMarkers.add( camera, mUi );
	addAmmo();															// 右下の弾数
	mHud.draw( session, dayCondition, searchProgress, waitingKind );	// 真ん中の照準「+」は出さない
}
