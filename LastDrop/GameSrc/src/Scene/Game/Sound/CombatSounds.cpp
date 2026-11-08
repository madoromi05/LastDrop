#include "GameSrc/Scene/Game/Sound/CombatSounds.h"
#include "GameSrc/Scene/Game/World/CombatResolver.h"

CombatSounds::CombatSounds( AudioEngine* audio ) :
	mZombieHits( audio ),
	mSurfaceHits( audio ),
	mAttacks( audio ) {
}

void CombatSounds::playHits( const CombatResolver& combat ) {
	mZombieHits.play( combat.zombieHitsThisTick() );
	mSurfaceHits.play( combat.surfaceHitsThisTick() );
}

void CombatSounds::playAttacks( const CombatResolver& combat ) {
	mAttacks.play( combat.zombieAttacksThisTick() );
}
