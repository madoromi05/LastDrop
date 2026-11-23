#ifndef INCLUDED_AIM_PUNCH_H
#define INCLUDED_AIM_PUNCH_H

#include "GameLib/Math/Random.h"

// 傷ついた瞬間の怯み（エイムパンチ）で、狙いへ足す上下・左右の角度を決める
// punch で上と左右ランダムへ跳ね、その後は時間で 0 へ戻る。弾の向きも一緒に動く（攻撃された瞬間は狙いが外れる）
// 何で傷ついたかは知らない（呼ぶのは持ち主の FirstPersonControl）。跳ねる量・戻る速さは AimPunch.cpp の定数で直す
class AimPunch{
public:
	explicit AimPunch( int seed );	// seed: 左右の向きの乱数の種

	void punch();	// 狙いを跳ねさせる。戻りきる前に呼ぶと、今のずれに足す
	void update();	// 1ティック進める（0 へ戻す）
	// 狙いへ足す角度（度。FirstPersonCamera と同じ約束で、ピッチは負が見上げる・ヨーは正が左）
	float pitchOffset() const;
	float yawOffset() const;

private:
	GameLib::Math::Random mRandom;
	float mRise; // 上へ跳ねている角度（度。正が上）
	float mSide; // 左右へ跳ねている角度（度。正が左）
};

#endif
