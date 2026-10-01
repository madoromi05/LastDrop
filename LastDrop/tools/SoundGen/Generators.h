#ifndef INCLUDED_SOUNDGEN_GENERATORS_H
#define INCLUDED_SOUNDGEN_GENERATORS_H

#include "Synth.h"
#include "SoundCatalog.h"

// 音の種類ごとの合成レシピ。random は揺らぎ（同じ音の別テイク）ごとに違う種で渡される。
// 返す音は正規化済み（大きさの釣り合いはゲーム側の音量で取る）。

// 足音（FootstepSynth.cpp）。id は SOUND_FOOTSTEP_* のどれか。
Synth::Buffer makeFootstep( SoundId id, Synth::Random* random );

// 装備・動作音（FoleySynth.cpp）。
Synth::Buffer makeClothRustle( Synth::Random* random );
Synth::Buffer makeWeaponDraw( Synth::Random* random );
Synth::Buffer makeWeaponHolster( Synth::Random* random );
Synth::Buffer makeAimIn( Synth::Random* random );
Synth::Buffer makeAimOut( Synth::Random* random );

// 環境音（AmbienceSynth.cpp）。ループで鳴らすものは継ぎ目なくつながる長さで返す。
Synth::Buffer makeWind( Synth::Random* random );
Synth::Buffer makeBirds( Synth::Random* random );
Synth::Buffer makeInsects( Synth::Random* random );
Synth::Buffer makeGenerator( Synth::Random* random );
Synth::Buffer makeSiren( Synth::Random* random );
Synth::Buffer makeAirplane( Synth::Random* random );

#endif
