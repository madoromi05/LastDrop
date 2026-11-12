#ifndef INCLUDED_ORTHO_BASIS_H
#define INCLUDED_ORTHO_BASIS_H

#include "GameLib/Math/Vector3.h"

// 1 本の向きに直交する 2 本の向きを作る、ただ1か所。状態は持たない
// 画面に平行な板（砂ぼこり・血の滴・マズルフラッシュ）は視線から、面に貼る絵（弾痕・血の跡）は面の法線から作る
class OrthoBasis{
public:
	// axis（長さ 1）に直交する right と up を作る（どちらも長さ 1。right = 仮の上 × axis、up = axis × right）
	// 仮の上はワールドの +Y。axis がほぼ真上・真下のときは外積が 0 に近づいて向きが決まらないので、+X を使う
	static void around( const GameLib::Math::Vector3& axis, GameLib::Math::Vector3* right, GameLib::Math::Vector3* up );
};

#endif
