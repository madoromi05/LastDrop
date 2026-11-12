#ifndef INCLUDED_COMBAT_SOUNDS_H
#define INCLUDED_COMBAT_SOUNDS_H

#include "GameSrc/Scene/Game/Sound/ZombieHitSounds.h"
#include "GameSrc/Scene/Game/Sound/SurfaceHitSounds.h"
#include "GameSrc/Scene/Game/Sound/ZombieAttackSounds.h"

class AudioEngine;
class CombatResolver;

// 戦闘の結果の効果音をまとめて鳴らす。どれも「このティックに起きた出来事（CombatResolver）」を読んで鳴らすだけ
// 弾がゾンビに当たった音・弾が物（建物/地面/車）に当たった音・ゾンビに攻撃された音の3種類
// カメラの揺れ・攻撃された見せ方（赤み/暗転）・足音は別系統（GameScene の shakeOnHeavyHits・PlayerDamageView・PlayerBodySounds）
class CombatSounds{
public:
	explicit CombatSounds( AudioEngine* audio );		// audio: 借りるだけ（所有しない）。この CombatSounds より長く生きていること。nullptr 不可
	void playHits( const CombatResolver& combat );		// 撃った結果の命中音（ゾンビ・物）。update の命中フィードバックで、世界を進めた直後に 1 回（次の update で出来事が入れ替わる前）
	void playAttacks( const CombatResolver& combat );	// 攻撃された音（つかむ・噛む・引っかく）。updatePlayerDamage で、世界を進めた後に 1 回

private:
	ZombieHitSounds mZombieHits;   // 弾がゾンビに当たった音
	SurfaceHitSounds mSurfaceHits; // 弾が物に当たった音
	ZombieAttackSounds mAttacks;   // ゾンビがつかんだ・噛んだ・引っかいた音
};

#endif
