#include "GameSrc/Scene/Game/Sound/ZombieHitSounds.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"
#include <algorithm>

namespace{
	// 弾が当たった音は 1 つ・1 つの大きさ（重さで音も音量も変えない）
	// 銃声（PlayerFoley）と同じく素材そのままの大きさで鳴らし、遠さによる小ささは目録の聞こえる距離に任せる
	const float kHitVolume = 1.f;
	const float kBodyFallVolume = 1.f;
	// 倒してから体が地面に着くまで（秒。death1/death2 を見て合わせる初期値）
	const float kBodyFallDelaySeconds = 0.6f;
	const int kBodyFallDelayTicks = GameTime::secondsToTicks( kBodyFallDelaySeconds );
}

ZombieHitSounds::ZombieHitSounds( AudioEngine* audio ) :
	mAudio( audio ) {
	ASSERT( mAudio && "ZombieHitSounds : audio is null." );
}

void ZombieHitSounds::play( const std::vector< CombatResolver::ZombieHit >& hits ) {
	updatePendingFalls();
	for ( const CombatResolver::ZombieHit& hit : hits ) {
		mAudio->play( SOUND_HIT_FLESH, AudioEngine::BUS_EFFECT, hit.point, kHitVolume );
		if ( hit.killed ) {
			PendingFall fall = { hit.point, kBodyFallDelayTicks };
			mPendingFalls.push_back( fall );
		}
	}
}

void ZombieHitSounds::updatePendingFalls() {
	for ( PendingFall& fall : mPendingFalls ) {
		if ( --fall.ticksLeft <= 0 ) {
			mAudio->play( SOUND_ZOMBIE_BODYFALL, AudioEngine::BUS_EFFECT, fall.point, kBodyFallVolume );
		}
	}
	mPendingFalls.erase( std::remove_if( mPendingFalls.begin(), mPendingFalls.end(),
		[]( const PendingFall& fall ) { return fall.ticksLeft <= 0; } ), mPendingFalls.end() );
}
