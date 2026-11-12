#ifndef INCLUDED_ZOMBIE_ATTACK_SOUNDS_H
#define INCLUDED_ZOMBIE_ATTACK_SOUNDS_H

#include "GameSrc/Scene/Game/World/CombatResolver.h"
#include <vector>

class AudioEngine;

// ゾンビが自キャラをつかんだ・噛んだ音を鳴らす係。世界（GameWorld）は音を知らないので、世界が出した
// 「このティックの攻撃の出来事」を読んで鳴らす（弾が当たった音の ZombieHitSounds と同じ立場）
// ゾンビの頭の所から 3D で鳴らす。素材に噛む音・ゾンビの声が無いので、今ある音を仮に使う
// （つかむ＝服の擦れ、噛む＝肉に当たった音を低くしたもの。空振りは鳴らさない。SummaryPlan/個別仕様/ゾンビ/ZombieAttack_Plan.txt の 5 章）
class ZombieAttackSounds{
public:
	explicit ZombieAttackSounds( AudioEngine* audio );	// audio: 鳴らす先。所有権は受け取らない。nullptr 不可。この係より長く生きていること

	void play( const std::vector< CombatResolver::ZombieAttackEvent >& events );	// events の出来事1つにつき1回鳴らす。1ティックに1回、GameWorld::update の後に呼ぶ（GameWorld::combat().zombieAttacksThisTick を渡す）

private:
	AudioEngine* mAudio; // 借りているだけ
};

#endif
