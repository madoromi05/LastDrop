#ifndef INCLUDED_ZOMBIE_HIT_SOUNDS_H
#define INCLUDED_ZOMBIE_HIT_SOUNDS_H

#include "GameSrc/Scene/Game/World/CombatResolver.h"
#include "GameLib/Math/Vector3.h"
#include <vector>

class AudioEngine;

// 弾がゾンビに当たった音を鳴らす係。世界（GameWorld）は音を知らないので、世界が出した「このティックの命中」を読んで鳴らす
// （自キャラの音を鳴らす PlayerBodySounds と同じ立場）。
// 当たった所から 3D で鳴らす。弾が当たった音は 1 つだけ（頭も同じ。2026-11-06 ユーザー指定。頭の手応えは血の量で出す）。
// 倒したときは、少し遅れて体が地面に着く音を鳴らす（UI の「倒した音」ではなく世界の中の音。HitFeel_Plan.txt Q2）。
// どの素材を使うか・聞こえる距離・音程の揺らぎは音の目録（SoundCatalog）が持つ。
class ZombieHitSounds{
public:
	// audio: 鳴らす先。所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	explicit ZombieHitSounds( AudioEngine* audio );

	// hits の命中1つにつき1回鳴らす。1ティックに1回、GameWorld::update の後に呼ぶ（GameWorld::combat().zombieHitsThisTick を渡す）。
	void play( const std::vector< CombatResolver::ZombieHit >& hits );

private:
	// 倒れる音を鳴らすまでの待ち（倒れるアニメで体が地面に着く頃。アニメの印を読む仕組みが無いので時間で決める）。
	struct PendingFall{
		GameLib::Math::Vector3 point;
		int ticksLeft;
	};
	// 待っている倒れる音を 1 ティック進め、時間が来た物を鳴らす。
	void updatePendingFalls();

	AudioEngine* mAudio; // 借りているだけ
	std::vector< PendingFall > mPendingFalls; // 毎ティック確保し直さないために持つ
};

#endif
