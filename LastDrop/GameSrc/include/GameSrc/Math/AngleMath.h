#ifndef INCLUDED_ANGLE_MATH_H
#define INCLUDED_ANGLE_MATH_H

#include "GameLib/Math/Vector3.h"

// 水平の向き（度）の決まりごとの、ただ1か所。状態は持たない
// 向き 0 は +Z、90 は +X（PlacementMatrix・Matrix34::rotateY と同じ回り方）。自キャラ・ゾンビ・カメラのヨーが同じ約束を使う
// 折り返す範囲は [-180, 180)。同じ向きの表し方を1つに決めるため（180 と -180 が両方出てこない）
class AngleMath{
public:
	static float wrapSigned( float degree );							// degree を [-180, 180) へ折り返す。何周ずれていても1回で計算する
	static float difference( float target, float current );				// current から target へ回るときの、近い回り方の角度（[-180, 180)。正なら +X 側へ回る）
	static float headingOf( const GameLib::Math::Vector3& direction );	// direction の水平の向き（x・z だけを見る）を度で返す。x・z が両方 0 なら 0（向きが無い）
};

#endif
