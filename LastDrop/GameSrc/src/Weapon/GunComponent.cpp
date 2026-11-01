#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Interaction/Inventory.h"

#include <algorithm>

GunComponent::GunComponent() :
	mFiredThisTick( false ),
	mDryFiredThisTick( false ),
	mDryFireLatched( false ) {
}

void GunComponent::pullTrigger( bool pulling, bool triggerHeld ) {
	mFiredThisTick = false;
	mDryFiredThisTick = false;
	if ( !triggerHeld ) {
		mDryFireLatched = false;
	}
	// 空撃ちの後は、離すまで引き金を引いていないことにする（押し続けてもカチッは 1 回。実銃も撃鉄が落ちるのは 1 回）。
	if ( !mFire.update( pulling && !mDryFireLatched ) ) {
		return;
	}
	if ( mMagazine.isEmpty() ) {
		mDryFiredThisTick = true;
		mDryFireLatched = true;
		return;
	}
	mMagazine.takeRound();
	mFiredThisTick = true;
}

void GunComponent::updateRecoil() {
	mRecoil.update( mFiredThisTick );
}

float GunComponent::recoilKick() const {
	return mRecoil.kick();
}

bool GunComponent::canStartReload( const Inventory& inventory ) const {
	return !mReload.isActive() && mMagazine.missingRounds() > 0 && inventory.count( ITEM_AMMO ) > 0;
}

void GunComponent::startReload() {
	mReload.start();
}

void GunComponent::cancelReload() {
	mReload.cancel();
}

void GunComponent::updateReload( Inventory* inventory ) {
	if ( !mReload.update() ) {
		return;
	}
	// 込め終えた。予備から弾倉の足りない分だけ移す（予備が足りなければ、ある分だけ）。
	int count = std::min( mMagazine.missingRounds(), inventory->count( ITEM_AMMO ) );
	for ( int i = 0; i < count; ++i ) {
		inventory->remove( ITEM_AMMO );
	}
	mMagazine.load( count );
}

const WeaponReload& GunComponent::reload() const {
	return mReload;
}

bool GunComponent::firedThisTick() const {
	return mFiredThisTick;
}

bool GunComponent::dryFiredThisTick() const {
	return mDryFiredThisTick;
}

bool GunComponent::isReloading() const {
	return mReload.isActive();
}

WeaponReload::Cue GunComponent::reloadCueThisTick() const {
	return mReload.cueThisTick();
}

int GunComponent::magazineRounds() const {
	return mMagazine.rounds();
}
