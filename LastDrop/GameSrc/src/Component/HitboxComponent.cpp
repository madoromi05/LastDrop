#include "GameSrc/Component/HitboxComponent.h"
#include "GameSrc/Hit/HitWorld.h"

#include "GameLib/GameLib.h"

HitboxComponent::HitboxComponent( const HitboxLayout* layout ) :
	mHitboxes( layout ) {
	ASSERT( layout && "HitboxComponent : layout is null." );
}

HitboxComponent::HitboxComponent( const SkinnedMeshData* data, const char* boneNamePrefix ) :
	mOwnedLayout( std::make_unique< HitboxLayout >( data, boneNamePrefix ) ),
	mHitboxes( mOwnedLayout.get() ) { // mOwnedLayout を先に作ってから借りる（メンバの宣言順）
}

const HitboxSet& HitboxComponent::hitboxes() const {
	return mHitboxes;
}

void HitboxComponent::addTo( CollisionWorld*, HitWorld* hitWorld ) {
	ASSERT( hitWorld && "HitboxComponent : world is null." );
	hitWorld->addTarget( this );
}

void HitboxComponent::removeFrom( CollisionWorld*, HitWorld* hitWorld ) {
	ASSERT( hitWorld && "HitboxComponent : world is null." );
	hitWorld->removeTarget( this );
}
