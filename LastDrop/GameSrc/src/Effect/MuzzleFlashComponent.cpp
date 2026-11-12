#include "GameSrc/Effect/MuzzleFlashComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/World/GameObject.h"

MuzzleFlashComponent::MuzzleFlashComponent() :
	mGun( nullptr ) {
}

MuzzleFlash& MuzzleFlashComponent::flash() {
	return mFlash;
}

const MuzzleFlash& MuzzleFlashComponent::flash() const {
	return mFlash;
}

UpdatePhase MuzzleFlashComponent::updatePhase() const {
	return UpdatePhase::Late;
}

void MuzzleFlashComponent::start() {
	mGun = requireComponent< GunComponent >();
}

void MuzzleFlashComponent::update() {
	// 前に撃った光を先に進めてから、このティックに撃った光を灯す（撃ったティックの描画が光り始めになる）
	mFlash.update();
	if ( mGun->firedThisTick() ) {
		mFlash.fire();
	}
}
