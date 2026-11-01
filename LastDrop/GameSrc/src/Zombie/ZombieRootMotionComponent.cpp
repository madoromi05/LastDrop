#include "GameSrc/Zombie/ZombieRootMotionComponent.h"
#include "GameSrc/Component/SkinnedModelComponent.h"
#include "GameSrc/Component/CharacterColliderComponent.h"
#include "GameSrc/World/GameObject.h"

#include "GameLib/GameLib.h"

ZombieRootMotionComponent::ZombieRootMotionComponent( const ZombieModel* model, const ZombieAnimationNames& names ) :
	mModel( model ),
	mNames( names ) {
	ASSERT( model && "ZombieRootMotionComponent : model is null." );
}

ZombieRootMotionComponent::~ZombieRootMotionComponent() {
}

void ZombieRootMotionComponent::start() {
	SkinnedMeshInstance* mesh = &requireComponent< SkinnedModelComponent >()->mesh();
	const CharacterMover* mover = &requireComponent< CharacterColliderComponent >()->mover();
	mSpan = std::make_unique< ZombieRootMotionSpan >( mesh, mover );
	mAnimations = std::make_unique< ZombieAnimationSet >( mModel, mNames, mesh, mSpan.get() );
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
