#ifndef INCLUDED_PLAYER_BODY_SOUNDS_H
#define INCLUDED_PLAYER_BODY_SOUNDS_H

#include "GameSrc/Audio/GroundMaterial.h"
#include "GameSrc/Audio/FootstepEmitter.h"
#include "GameSrc/Audio/PlayerFoley.h"
#include <vector>

class AudioEngine;
class IGroundSurface;
class PlayerCharacter;

// 自キャラの体から鳴る音（足音・装備の音）の係。自キャラの状態を読んで、鳴らす係（FootstepEmitter / PlayerFoley）へ渡す。
// 足音の音色を決める「足元の床の材質」の表（GroundMaterialMap）もここが持つ。
// PlayerCharacter は音を知らず、鳴らす係はキャラのクラスを知らない。その2つをつなぐのがここだけなので、
// キャラの状態の読み方や、音を鳴らす高さを変えても、どちらにも波及しない。
class PlayerBodySounds{
public:
	// audio: 鳴らす先。ground: 足元の材質を答える地面。どちらも所有権は受け取らない。nullptr 不可。この係より長く生きていること。
	// zones: 床の材質を地面と変える所（水たまりなど）。写して持つ。
	PlayerBodySounds( AudioEngine* audio, const IGroundSurface* ground, const std::vector< GroundZone >& zones );

	// 足音・装備の音を鳴らす。1ティックに1回、キャラの更新の後に呼ぶ。
	void update( const PlayerCharacter& player );

private:
	GroundMaterialMap mGround; // 足元の床の材質（地面の部品の材質に、zones の長方形を重ねたもの）
	FootstepEmitter mFootsteps;
	PlayerFoley mFoley;
};

#endif
