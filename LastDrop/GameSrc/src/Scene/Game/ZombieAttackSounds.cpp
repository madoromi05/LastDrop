#include "GameSrc/Scene/Game/ZombieAttackSounds.h"
#include "GameSrc/Audio/AudioEngine.h"

#include "GameLib/GameLib.h"

namespace{
	// 音量。ほかの 3D の音と同じく素材そのままの大きさで鳴らし、遠さによる小ささは目録の聞こえる距離に任せる。
	const float kVolume = 1.f;
	// 腕が当たった音は、弾が肉に当たった音を低くして、重い打撃に寄せる（仮。素材が来たら差し替える）。
	const float kHitPitch = 0.8f;
}

ZombieAttackSounds::ZombieAttackSounds( AudioEngine* audio ) :
	mAudio( audio ) {
	ASSERT( mAudio && "ZombieAttackSounds : audio is null." );
}

void ZombieAttackSounds::play( const std::vector< GameWorld::ZombieAttackEvent >& events ) {
	for ( const GameWorld::ZombieAttackEvent& event : events ) {
		mAudio->play( SOUND_HIT_FLESH, AudioEngine::BUS_EFFECT, event.point, kVolume, kHitPitch );
	}
}
