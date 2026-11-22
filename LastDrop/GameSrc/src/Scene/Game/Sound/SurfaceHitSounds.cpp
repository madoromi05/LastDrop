#include "GameSrc/Scene/Game/Sound/SurfaceHitSounds.h"
#include "GameSrc/Audio/AudioEngine.h"

#include "GameLib/GameLib.h"

namespace{
	const float kImpactVolume = 1.f;	// 音量。ゾンビに当たった音と同じく素材そのままの大きさで鳴らし、遠さによる小ささは目録の聞こえる距離に任せる

	// 材質ごとの着弾音。並びは GroundMaterial と同じ
	// 今の町にある物（建物・歩道＝コンクリート、車道＝砂利、車・缶＝金属）の音だけを持ち、音の無い材質（木）は近い音を借りる
	const SoundId kImpactSounds[] = {
		SOUND_IMPACT_CONCRETE, // コンクリート
		SOUND_IMPACT_METAL,    // 金属
		SOUND_IMPACT_CONCRETE, // 木（木の音が無いので、硬い音を借りる）
		SOUND_IMPACT_DIRT,     // 砂利（車道）
	};
	static_assert( sizeof( kImpactSounds ) / sizeof( kImpactSounds[ 0 ] ) == GROUND_NUM, "kImpactSounds must list every GroundMaterial" );
}

SurfaceHitSounds::SurfaceHitSounds( AudioEngine* audio ) :
	mAudio( audio ) {
	ASSERT( mAudio && "SurfaceHitSounds : audio is null." );
}

void SurfaceHitSounds::play( const std::vector< CombatResolver::SurfaceHit >& hits ) {
	for ( const CombatResolver::SurfaceHit& hit : hits ) {
		mAudio->play( kImpactSounds[ hit.material ], AudioEngine::BUS_EFFECT, hit.point, kImpactVolume );
	}
}
