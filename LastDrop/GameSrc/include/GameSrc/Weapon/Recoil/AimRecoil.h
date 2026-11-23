#ifndef INCLUDED_AIM_RECOIL_H
#define INCLUDED_AIM_RECOIL_H

#include "GameLib/Math/Random.h"

// 撃った反動による弾道のずれ（狙いへ足す上下・左右の角度）を決める
// 連射のたびにたまっていき、マウスを動かさなければ上へ上がりながら左右にふらつく。上は数発で頭打ちになる
// 撃ちやめると時間で 0 へ戻る（マウスで決めた向きそのものは変えないので、狙いは元の所へ戻る）
// 1 発ごとに跳ねて戻る視点の反動（ViewRecoil）とは別の量で、カメラへ足すのは持ち主（FirstPersonControl）が両方を合わせて行う
// 上がる量・ぶれる量・戻る速さを変えるときは AimRecoil.cpp の定数だけを直す
class AimRecoil{
public:
	explicit AimRecoil( int seed );	// seed: 左右のぶれの乱数の種

	void update( float shotRecoil );	// 1ティック進める。shotRecoil: このティックに1発撃ったならその銃の反動の大きさ（WeaponSpec::recoilScale。ライフルが 1）、撃っていなければ 0
	float pitchOffset() const;			// 狙いの上下へ足す角度（度。FirstPersonCamera のピッチと同じ約束で、負が見上げる向き）
	float yawOffset() const;			// 狙いの左右へ足す角度（度。FirstPersonCamera のヨーと同じ約束で、正が左）

private:
	void kick( float scale );	// 撃ったティックに、連射の何発目かに合わせて上下・左右へ足す。scale: 足す量に掛ける反動の大きさ

	GameLib::Math::Random mRandom;
	float mRise;       // 上がっている角度（度。正が上）
	float mSide;       // 左右にずれている角度（度。正が左）
	int mShotIndex;    // 今の連射の何発目か（撃ちやめたら 0 に戻る）
	int mQuietTicks;   // 最後に撃ってからのティック数
};

#endif
