#include "GameSrc/Scene/Game/SurfaceHitSounds.h"
#include "GameSrc/Audio/AudioEngine.h"

#include "GameLib/GameLib.h"

namespace{
	// 音量。ゾンビに当たった音と同じく素材そのままの大きさで鳴らし、遠さによる小ささは目録の聞こえる距離に任せる。
	const float kImpactVolume = 1.f;

	// 材質ごとの着弾音。並びは GroundMaterial と同じ。
	// 今の町にある物（建物・歩道＝コンクリート、車道＝砂利、車・缶＝金属）の音だけを持ち、町に無い材質は近い音を借りる。
	const SoundId kImpactSounds[] = {
		SOUND_IMPACT_CONCRETE, // コンクリート
		SOUND_IMPACT_METAL,    // 金属
		SOUND_IMPACT_CONCRETE, // 木（木の音が無いので、硬い音を借りる）
		SOUND_IMPACT_DIRT,     // 草
		SOUND_IMPACT_DIRT,     // 砂利（車道）
		SOUND_IMPACT_DIRT,     // 水たまり（水の音が無いので、柔らかい音を借りる）
		SOUND_IMPACT_CONCRETE, // ガラス（ガラスの音が無いので、硬い音を借りる）
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
