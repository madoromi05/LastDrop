#ifndef INCLUDED_IRAY_BLOCKER_H
#define INCLUDED_IRAY_BLOCKER_H

#include "GameLib/Math/Vector3.h"

// 弾を止められる物（建物・壁など、部位を持たない物）。HitWorld に登録すると、弾の線分の問い合わせに答える。
// 部位を持つキャラは IHittable の方で答える（どの部位に当たったかが要るため）。
class IRayBlocker{
public:
	virtual ~IRayBlocker(){}
	// ワールドの線分 origin〜origin+delta が一番早く当たる所の時刻（0〜1）と、面の法線（ワールド・長さ1。線分の来た側を向く）。
	// 当たらなければ false。time / normal: 出力先。nullptr 不可。
	virtual bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const = 0;
};

#endif
