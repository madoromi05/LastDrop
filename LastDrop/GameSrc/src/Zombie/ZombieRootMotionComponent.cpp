#include "GameSrc/Zombie/ZombieRootMotionComponent.h"
#include "GameSrc/Zombie/ZombieModelComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObject.h"

ZombieRootMotionComponent::ZombieRootMotionComponent( const ZombieAnimationNames& names ) :
	mNames( names ) {
}

ZombieRootMotionComponent::~ZombieRootMotionComponent() {
}

void ZombieRootMotionComponent::awake() {
	const ZombieModel* model = requireComponent< ZombieModelComponent >()->model();
	SkinnedMeshInstance* mesh = &requireComponent< SkinnedModelComponent >()->mesh();
	const CharacterMover* mover = &requireComponent< CharacterColliderComponent >()->mover();
	mSpan = std::make_unique< ZombieRootMotionSpan >( mesh, mover );
	mAnimations = std::make_unique< ZombieAnimationSet >( model, mNames, mesh, mSpan.get() );
}

ZombieRootMotionSpan& ZombieRootMotionComponent::span() {
	return *mSpan;
}

const ZombieRootMotionSpan& ZombieRootMotionComponent::span() const {
	return *mSpan;
}

ZombieAnimationSet& ZombieRootMotionComponent::animations() {
	return *mAnimations;
}

const ZombieAnimationSet& ZombieRootMotionComponent::animations() const {
	return *mAnimations;
}
