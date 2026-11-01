#include "GameSrc/Render/SkinnedMesh/BonePoseModifierChain.h"

#include "GameLib/GameLib.h"

void BonePoseModifierChain::add( BonePoseModifier* modifier ) {
	ASSERT( modifier && "BonePoseModifierChain : modifier is null." );
	mModifiers.push_back( modifier );
}

void BonePoseModifierChain::clear() {
	mModifiers.clear();
}

void BonePoseModifierChain::apply( std::vector< GameLib::Math::Matrix34 >* pose ) {
	for ( BonePoseModifier* modifier : mModifiers ) {
		modifier->apply( pose );
	}
}
