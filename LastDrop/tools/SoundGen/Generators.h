#ifndef INCLUDED_SOUNDGEN_GENERATORS_H
#define INCLUDED_SOUNDGEN_GENERATORS_H

#include "Synth.h"
#include "GameSrc/Audio/SoundCatalog.h"

// 音の種類ごとの合成レシピ。random は揺らぎ（同じ音の別テイク）ごとに違う種で渡される
// 返す音は正規化済み（大きさの釣り合いはゲーム側の音量で取る）

// 足音（FootstepSynth.cpp）。id は SOUND_FOOTSTEP_* のどれか
Synth::Buffer makeFootstep( SoundId id, Synth::Random* random );

// 装備・動作音（FoleySynth.cpp）
Synth::Buffer makeClothRustle( Synth::Random* random );
Synth::Buffer makeWeaponDraw( Synth::Random* random );
Synth::Buffer makeWeaponHolster( Synth::Random* random );
Synth::Buffer makeAimIn( Synth::Random* random );
Synth::Buffer makeAimOut( Synth::Random* random );
// 銃の懐中電灯のスイッチ「カチッ」（押す音と戻る音）
Synth::Buffer makeFlashlightClick( Synth::Random* random );

// 環境音（AmbienceSynth.cpp）。今はサイレンだけ
Synth::Buffer makeSiren( Synth::Random* random );

// 着弾音（ImpactSynth.cpp）。id は SOUND_IMPACT_* のどれか
Synth::Buffer makeImpact( SoundId id, Synth::Random* random );

// 漁る音（LootSynth.cpp）。id は SOUND_LOOT_SEARCH_* のどれか（開けている間のループの音）
Synth::Buffer makeLootSearch( SoundId id, Synth::Random* random );

// 脱出の車のエンジン（EngineSynth.cpp。FuelEscapeLoading_Plan.txt Q5）。掛ける音（1 回）と、アイドリング（ループ）
Synth::Buffer makeEngineStart( Synth::Random* random );
Synth::Buffer makeEngineIdle( Synth::Random* random );
// 給油している間のガソリンを注ぐ音（PourSynth.cpp。ループ。FuelEscapeLoading_Plan.txt 改訂 2）
Synth::Buffer makeRefuelPour( Synth::Random* random );
// 倒れたゾンビの体が地面に着く「ドサッ」（BodyFallSynth.cpp。HitFeel_Plan.txt Q5）
Synth::Buffer makeZombieBodyFall( Synth::Random* random );

#endif
