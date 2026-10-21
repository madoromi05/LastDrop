#include "GameSrc/Scene/Game/ZombieAttackSounds.h"
#include "GameSrc/Audio/AudioEngine.h"

#include "GameLib/GameLib.h"

namespace{
	// 音量。ほかの 3D の音と同じく素材そのままの大きさで鳴らし、遠さによる小ささは目録の聞こえる距離に任せる。
	const float kVolume = 1.f;
	const float kGrabPitch = 1.f;
	// 噛む音は、弾が肉に当たった音を低くして、重く湿った音に寄せる（仮。噛む音の素材が来たら差し替える）。
	const float kBitePitch = 0.8f;
}

ZombieAttackSounds::ZombieAttackSounds( AudioEngine* audio ) :
	mAudio( audio ) {
	ASSERT( mAudio && "ZombieAttackSounds : audio is null." );
}

void ZombieAttackSounds::play( const std::vector< GameWorld::ZombieAttackEvent >& events ) {
	for ( const GameWorld::ZombieAttackEvent& event : events ) {
		switch ( event.kind ) {
			case GameWorld::ZombieAttackEvent::Kind::Grab:
				mAudio->play( SOUND_CLOTH_RUSTLE, AudioEngine::BUS_EFFECT, event.point, kVolume, kGrabPitch );
				break;
			case GameWorld::ZombieAttackEvent::Kind::Bite:
				mAudio->play( SOUND_HIT_FLESH, AudioEngine::BUS_EFFECT, event.point, kVolume, kBitePitch );
				break;
		}
	}
}
