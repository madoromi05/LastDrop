#include "GameSrc/Effect/MuzzleFlashComponent.h"
#include "GameSrc/Weapon/GunComponent.h"
#include "GameSrc/Component/ILightSocket.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

MuzzleFlashComponent::MuzzleFlashComponent() :
	mGun( nullptr ),
	mSocket( nullptr ) {
}

MuzzleFlash& MuzzleFlashComponent::flash() {
	return mFlash;
}

void MuzzleFlashComponent::setSocket( const ILightSocket* socket ) {
	mSocket = socket;
}

bool MuzzleFlashComponent::lightSource( LightSource* out ) const {
	ASSERT( out && "MuzzleFlashComponent : out is null." );
	Vector3 muzzlePosition;
	Vector3 muzzleDirection;
	if ( !mSocket || !mSocket->lightSocket( &muzzlePosition, &muzzleDirection ) ) {
		return false;
	}
	return mFlash.light( muzzlePosition, muzzleDirection, out );
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
