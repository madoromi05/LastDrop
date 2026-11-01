#ifndef INCLUDED_BULLET_H
#define INCLUDED_BULLET_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Hit/HitWorld.h"

class StaticMeshData;
class IHittable;

// 飛んでいる弾1発（7.62 NATO の弾頭）。撃ち出した向きへまっすぐ、決まった速さで飛ぶ（落下はしない）。
// 1ティックに進む分の線分を命中の名簿（HitWorld）に聞き、何かに当たるか、届く距離を飛びきったら終わる（isFinished）。
// 弾は速い（1ティックに約 14m）ので、点ではなく線分で聞く。点で聞くと、薄い壁やゾンビを飛び越えてしまうため。
// 当たった相手をどうするか（ダメージ・弾痕）は持たない。当たった所（HitWorld::Hit）を持ち主へ返して消える。
// 名簿には載らない（弾に弾は当たらず、移動も止めない）。持ち主は BulletList。
class Bullet{
public:
	// origin: 撃ち出す位置（ワールド座標）。direction: 飛ぶ向き（長さ1）。
	Bullet( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& direction );

	// 1ティック進める。このティックに何かに当たったら hit に書いて true を返す（その後は終わった弾になる）。
	// 終わった弾に呼んでも何もしない（false）。
	// world: 当たりを聞く名簿。
	// shooter: 当てないキャラ（撃った本人。弾は本人の頭の箱の中から出るため）。所有権は受け取らない。nullptr なら全員に当たる。
	// hit: 当たった所の出力先。nullptr 不可。
	bool update( const HitWorld& world, const IHittable* shooter, HitWorld::Hit* hit );
	// data の形で、今の位置に、飛ぶ向きへ先を向けて描く。data: 弾頭（Resource/bullet_tip.txt。底が原点・先が +Y）。isReady() 済みであること。
	void draw( const StaticMeshData& data ) const;

	bool isFinished() const;	// 何かに当たったか、届く距離を飛びきったか（持ち主が消す）
	const GameLib::Math::Vector3& direction() const; // 飛ぶ向き（長さ1）

	// この弾1発の、部位の倍率を掛ける前のダメージ。当たった後に相手へ渡すのは CombatResolver。
	// 弾の性能なので、弾の速さと同じ所（Bullet.cpp）に置く。
	static float damage();

private:
	GameLib::Math::Vector3 mPosition;	// 弾の底の位置（ワールド座標）
	GameLib::Math::Vector3 mDirection;	// 飛ぶ向き（長さ1）
	float mTraveled;					// 撃ち出してから飛んだ距離（m）
	bool mFinished;
};

#endif
