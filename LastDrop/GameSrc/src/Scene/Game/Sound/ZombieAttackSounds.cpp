#include "GameSrc/Scene/Game/Sound/ZombieAttackSounds.h"
#include "GameSrc/Audio/AudioEngine.h"

#include "GameLib/GameLib.h"

namespace{
	const float kVolume = 1.f;		// 音量。ほかの 3D の音と同じく素材そのままの大きさで鳴らし、遠さによる小ささは目録の聞こえる距離に任せる
	const float kGrabPitch = 1.f;	// つかんだ音は、服の擦れをそのままの高さで
	const float kHitPitch = 0.8f;	// 噛む音は、弾が肉に当たった音を低くして、重く湿った音に寄せる（仮。噛む音の素材が来たら差し替える）
}

ZombieAttackSounds::ZombieAttackSounds( AudioEngine* audio ) :
	mAudio( audio ) {
	ASSERT( mAudio && "ZombieAttackSounds : audio is null." );
}

void ZombieAttackSounds::play( const std::vector< CombatResolver::ZombieAttackEvent >& events ) {
	for ( const CombatResolver::ZombieAttackEvent& event : events ) {
		switch ( event.kind ) {
			case CombatResolver::ZombieAttackEvent::Kind::Grab:
				mAudio->play( SOUND_CLOTH_RUSTLE, AudioEngine::BUS_EFFECT, event.point, kVolume, kGrabPitch );
				break;
			case CombatResolver::ZombieAttackEvent::Kind::Hit:
				mAudio->play( SOUND_HIT_FLESH, AudioEngine::BUS_EFFECT, event.point, kVolume, kHitPitch );
				break;
		}
	}
}
