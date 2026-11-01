#ifndef INCLUDED_ZOMBIE_HIT_SOUNDS_H
#define INCLUDED_ZOMBIE_HIT_SOUNDS_H

#include "GameSrc/Scene/Game/CombatResolver.h"
#include <vector>

class AudioEngine;

// 弾がゾンビに当たった音を鳴らす係。世界（GameWorld）は音を知らないので、世界が出した「このティックの命中」を読んで鳴らす
// （自キャラの音を鳴らす PlayerBodySounds と同じ立場）。
// 当たった所から 3D で鳴らす。頭なら硬い音、それ以外は体の音（2026-10-18 ユーザー指定。倒したときの音は作らない）。
// どの素材を使うか・聞こえる距離・音程の揺らぎは音の目録（SoundCatalog）が持つ。
class ZombieHitSounds{
public:
	// audio: 鳴らす先。所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	explicit ZombieHitSounds( AudioEngine* audio );

	// hits の命中1つにつき1回鳴らす。1ティックに1回、GameWorld::update の後に呼ぶ（GameWorld::combat().zombieHitsThisTick を渡す）。
	void play( const std::vector< CombatResolver::ZombieHit >& hits );

private:
	AudioEngine* mAudio; // 借りているだけ
};

#endif
