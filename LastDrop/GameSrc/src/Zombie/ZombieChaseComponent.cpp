#include "GameSrc/Zombie/ZombieChaseComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"

ZombieChaseComponent::ZombieChaseComponent( const NavMesh* navMesh, int sightPhase, int wanderSeed,
const ZombieFlinchLayout* swayLayout, float swayPeriodSeconds, float swayStartPhase ) :
	mNavMesh( navMesh ),
	mSightPhase( sightPhase ),
	mWanderSeed( wanderSeed ),
	mSway( swayLayout, swayPeriodSeconds, swayStartPhase ) {
	ASSERT( navMesh && "ZombieChaseComponent : navMesh is null." );
}

ZombieChaseComponent::~ZombieChaseComponent() {
}

void ZombieChaseComponent::start() {
	const CharacterColliderComponent* collider = requireComponent< CharacterColliderComponent >();
	// 仲間の並び（ZombieCrowd）に自分を名乗るのは、物そのもの（ポインタを比べるだけ）。
	mLocomotion = std::make_unique< ZombieLocomotion >( gameObject(), mNavMesh, &collider->mover(), mSightPhase, mWanderSeed );
}

ZombieLocomotion& ZombieChaseComponent::locomotion() {
	return *mLocomotion;
}

const ZombieLocomotion& ZombieChaseComponent::locomotion() const {
	return *mLocomotion;
}

ZombieSway& ZombieChaseComponent::sway() {
	return mSway;
}

ZombieChase::Mode ZombieChaseComponent::mode() const {
	return mLocomotion->mode();
}

const ZombiePath& ZombieChaseComponent::path() const {
	return mLocomotion->path();
}

const GameLib::Math::Vector3& ZombieChaseComponent::separation() const {
	return mLocomotion->separation();
}
