#include "GameSrc/Scene/Game/Hud/HudController.h"
#include "GameSrc/Scene/Game/World/GameWorld.h"
#include "GameSrc/Interaction/Item/Inventory.h"
#include "GameSrc/Player/Body/HoldComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameLib/Math/Vector3.h"

HudController::HudController( GameWorld* world, Inventory* inventory, UiRenderer* ui ) :
	mWorld( world ),
	mInventory( inventory ),
	mUi( ui ),
	mHud( ui ),
	mAmmoHud( ui ) {
}

void HudController::updateAmmoVisibility( bool wanted ) {
	mAmmoHudVisibility.update( wanted );
}

void HudController::updateMarkers( const GameLib::Math::Vector3& eye, const IInteractable* target ) {
	mInteractMarkers.update( mWorld->interaction(), eye, target, *mInventory );
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
	ammo.reserve = mInventory->count( gun->spec().ammo );
	mAmmoHud.add( ammo, mAmmoHudVisibility.opacity() );
}

void HudController::draw( const FirstPersonCamera& camera, const RaidSession& session, const DayConditionEffect& dayCondition,
	float searchProgress, RaidHud::ProgressKind waitingKind ) {
	// 積むだけ（1 回の描画で UiRenderer::draw がまとめて描く）
	mInteractMarkers.add( camera, mUi );
	addAmmo();															// 右下の弾数
	mHud.draw( session, dayCondition, searchProgress, waitingKind );	// 真ん中の照準「+」は出さない
}
