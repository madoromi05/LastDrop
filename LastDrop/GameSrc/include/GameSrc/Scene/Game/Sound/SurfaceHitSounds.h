#ifndef INCLUDED_SURFACE_HIT_SOUNDS_H
#define INCLUDED_SURFACE_HIT_SOUNDS_H

#include "GameSrc/Scene/Game/World/CombatResolver.h"
#include <vector>

class AudioEngine;

// 弾が物（建物・地面・車・拾える物）に当たった音（着弾音）を鳴らす係。世界（GameWorld）は音を知らないので、
// 世界が出した「このティックの着弾」を読んで鳴らす（ZombieHitSounds と同じ立場）。
// 当たった所から 3D で鳴らし、音は当たった物の材質で選ぶ（SummaryPlan/個別仕様/ImpactSound_Plan.txt）。
// ゾンビに当たった音とは別の係にする。読む物（部位と材質）も音の選び方も違い、
// タルコフも物への着弾と体への命中を別々に調整しているため。
// どの素材を使うか・聞こえる距離・音程の揺らぎは音の目録（SoundCatalog）が持つ。
class SurfaceHitSounds{
public:
	// audio: 鳴らす先。所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	explicit SurfaceHitSounds( AudioEngine* audio );

	// hits の着弾1つにつき1回鳴らす。1ティックに1回、GameWorld::update の後に呼ぶ（GameWorld::combat().surfaceHitsThisTick を渡す）。
	void play( const std::vector< CombatResolver::SurfaceHit >& hits );

private:
	AudioEngine* mAudio; // 借りているだけ
};

#endif
