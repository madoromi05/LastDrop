#ifndef INCLUDED_FREE_CAMERA_H
#define INCLUDED_FREE_CAMERA_H

#include "GameSrc/Player/Camera/FirstPersonCamera.h"

// 自キャラとは別の、町やゾンビを見て回る確認用のカメラ（Debug・Profile ビルドだけで使う）
// F6 の Debug タブの Free camera で使う・やめるを切り替える（DebugSwitches::toggleFreeCamera）。使っている間は:
//   マウスで向きを変える（カーソルロック中だけ）。真下近くまで見下ろせる
//   WASD で見ている方へ前後・左右、Space で上がり Ctrl で下がる。Shift を押している間はさらに速い
//   壁・地面はすり抜ける（当たり判定を持たない）
// 世界は止めない。自キャラはその場で立ち止まり、ゾンビは追う・うろつく・湧くを続ける（GameScene が自キャラへ「何もしない」を渡す）
//
// 自キャラの視点（FirstPersonControl）とは別に向きと位置を持つ。自キャラの視点は動かさないので、
// やめるとその場の自キャラの目に戻り、ゾンビの湧く場所の判定（自キャラの目と向きを使う）も変わらない
//
// 呼ぶ順（IScene の beginFrame → update → draw に合わせる）:
//   handleToggle・look（1回の描画につき1回）→ move（update のたび）→ apply（描く前）
class FreeCamera{
public:
	FreeCamera();

	// F6 から切り替えを頼まれていたら（DebugSwitches::toggleFreeCamera）、使う・やめるを切り替えて頼みを下ろす。1回の描画につき1回呼ぶ
	// 使い始めるときは start（自キャラの視点）の位置と向きから飛び始める（切り替えた瞬間に画が跳ばないように）
	void handleToggle( const FirstPersonCamera& start );
	bool isActive() const;

	// マウスの移動量で向きを変える。使っている間・カーソルロック中だけ。1回の描画につき1回
	// （update の回数に合わせると、2回進めた描画で2倍回るため。FirstPersonControl::readInput と同じ）
	void look();
	// キーで1ティックぶん動かす。使っている間だけ。update のたびに呼ぶ（速さをティックで決めているため）
	void move();
	// 射影・ビュー行列と視点位置を描画ステートへ設定する。自キャラの視点を置いた後、描く前に呼ぶ（上書きする）
	void apply() const;
	const FirstPersonCamera& camera() const;

private:
	FirstPersonCamera mCamera;
	bool mActive; // 使っているか
};

#endif
