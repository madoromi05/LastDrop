#ifndef INCLUDED_PLAYER_COMMAND_H
#define INCLUDED_PLAYER_COMMAND_H

// 自キャラへの1フレームぶんの操作（何をしたいか）。
// PlayerCharacter は入力デバイスを知らず、これだけを受け取って動く。
// 作るのはシーン（今は Pad から読む）。キー割り当て・パッド・AI・リプレイなど、
// 操作の出どころが変わっても PlayerCharacter には波及しない。
struct PlayerCommand{
	float forward;       // 前後の移動量（-1〜1。前が正）
	float strafe;        // 横の移動量（-1〜1。右が正）
	bool run;            // 走る（押されている間）
	bool holsterPressed; // 武器を出す／しまう（押した瞬間）
	bool aimHeld;        // 覗く（押されている間。短押し・長押しの判定は WeaponStance）
};

#endif
