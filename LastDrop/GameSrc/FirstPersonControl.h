#ifndef INCLUDED_FIRST_PERSON_CONTROL_H
#define INCLUDED_FIRST_PERSON_CONTROL_H

#include "FirstPersonCamera.h"
#include "PlayerCommand.h"
#include "GameLib/Math/Vector3.h"

// 一人称の操作の窓口。入力（Pad・マウス）を読んで、視点（FirstPersonCamera）を回し、自キャラへの操作（PlayerCommand）を作る。
// キー割り当てと「キャラの動き・カメラの向き」をつなぐのはここだけ（PlayerCharacter も FirstPersonCamera も入力デバイスを知らない）。
//
// マウスはカーソルロック（GameLib::Input::Manager）で読む。Esc で解除、クリックで再ロック。
// 解除中は視点を回さない（カーソルが自由に動く間の移動量を視点に入れると、画面外の操作で回ってしまうため）。
//
// 呼ぶ順（IScene の beginFrame → update → draw に合わせる）:
//   readInput（1回の描画につき1回）→ command を使ってキャラを動かす → consumePressed（update のたび）→ placeCamera（描く前）
class FirstPersonControl{
public:
	// startYaw: 始めに見る水平の向き（度。FirstPersonCamera::yaw と同じ約束）。キャラの向いている方を渡す。
	explicit FirstPersonControl( float startYaw );
	// シーンを抜けたらカーソルを返す（隠れたまま・閉じ込められたままにしない）。
	~FirstPersonControl();
	FirstPersonControl( const FirstPersonControl& ) = delete;
	FirstPersonControl& operator=( const FirstPersonControl& ) = delete;

	// カーソルをロックして、視点を回せるようにする（世界を並べ終えて操作を始めるときに呼ぶ）。
	void lockCursor();
	bool isCursorLocked() const;

	// 入力を読む。1回の描画につき1回、Pad::update の後に呼ぶ（update の回数に合わせると、2回進めた描画で視点が2倍回るため）。
	// カーソルロックの切り替え → ロック中ならマウスの移動量で視点を回す → 操作を command へまとめる。
	// aimWeight: 覗き具合（0〜1。PlayerCharacter::aimWeight）。ズーム中は視野角の比で感度を下げるのに使う。
	void readInput( float aimWeight );
	// 自キャラへの操作。押した瞬間の入力（holsterPressed）は、consumePressed で消すまで貯めてある。
	// 1回の描画の間に update が0回でも入力が消えず、2回あっても2回反応しないようにするため。
	const PlayerCommand& command() const;
	// 押した瞬間の入力を消す。command をキャラへ渡した後（update のたび）に呼ぶ。
	void consumePressed();

	// 視点を目へ置き、覗き具合に合わせた視野角にして、描画ステートへ反映する。描く前、キャラを動かした後に呼ぶ。
	// eye: 目の位置（ワールド座標。PlayerCharacter::eyePosition）。nullptr なら前に置いた位置のまま（キャラがいないとき）。
	// aimWeight: 覗き具合（0〜1）。覗いている間は視野を狭めて少しズームする。
	void placeCamera( const GameLib::Math::Vector3* eye, float aimWeight );
	const FirstPersonCamera& camera() const;

private:
	FirstPersonCamera mCamera;
	PlayerCommand mCommand;
};

#endif
