#ifndef INCLUDED_PLAYER_BODY_SOUNDS_H
#define INCLUDED_PLAYER_BODY_SOUNDS_H

#include "GameSrc/Audio/GroundMaterial.h"
#include "GameSrc/Audio/FootstepEmitter.h"
#include "GameSrc/Audio/PlayerFoley.h"

class AudioEngine;
class IGroundSurface;
class GameObject;

// 自キャラの体から鳴る音（足音・装備の音）の係。自キャラの状態を読んで、鳴らす係（FootstepEmitter / PlayerFoley）へ渡す
// 足音の音色を決める「足元の床の材質」の表（GroundMaterialMap）もここが持つ
// 自キャラの部品は音を知らず、鳴らす係はキャラの部品を知らない。その2つをつなぐのがここだけなので、
// キャラの状態の読み方や、音を鳴らす高さを変えても、どちらにも波及しない
class PlayerBodySounds{
public:
	// audio: 鳴らす先。ground: 足元の材質を答える地面。どちらも所有権は受け取らない。nullptr 不可。この係より長く生きていること
	PlayerBodySounds( AudioEngine* audio, const IGroundSurface* ground );

	// 足音・装備の音を鳴らす。1ティックに1回、キャラの更新の後に呼ぶ
	// player: 自キャラの物（Transform・PlayerLocomotion・Hold の部品を読む。手に持った銃があれば、その GunComponent・LightComponent も）
	void update( const GameObject& player );

private:
	GroundMaterialMap mGround; // 足元の床の材質（地面の部品の材質）
	FootstepEmitter mFootsteps;
	PlayerFoley mFoley;
};

#endif
