#include "GameSrc/Scene/Game/ZombieHitSounds.h"
#include "GameSrc/Audio/AudioEngine.h"

#include "GameLib/GameLib.h"

namespace{
	// 音量。銃声（PlayerFoley）と同じく素材そのままの大きさで鳴らし、遠さによる小ささは目録の聞こえる距離に任せる。
	const float kHitVolume = 1.f;
}

ZombieHitSounds::ZombieHitSounds( AudioEngine* audio ) :
	mAudio( audio ) {
	ASSERT( mAudio && "ZombieHitSounds : audio is null." );
}

void ZombieHitSounds::play( const std::vector< CombatResolver::ZombieHit >& hits ) {
	for ( const CombatResolver::ZombieHit& hit : hits ) {
		SoundId sound = ( hit.part == BodyPart::Head ) ? SOUND_HIT_HEAD : SOUND_HIT_FLESH;
		mAudio->play( sound, AudioEngine::BUS_EFFECT, hit.point, kHitVolume );
	}
}
