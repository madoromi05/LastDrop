#ifndef INCLUDED_BLOOD_SPRAY_H
#define INCLUDED_BLOOD_SPRAY_H

#include "GameLib/Math/Vector3.h"
#include <random>

// 弾がゾンビに当たった瞬間に飛ぶ血しぶき1回分。当たった点から血の滴を、弾の抜ける向きへ多く・撃った側へ少し飛ばし、
// 重力で弧を描いて落としながら広げて薄くし、決まった時間で終わる（isFinished）。
// 作りは砂ぼこり（ImpactDust）と同じだが、血は粉より重い液体なので、本当の重力で落とし、ぼかさずに濃く見せる。
// 滴の位置・大きさ・濃さを答えるだけで、描かない（まとめて描くのは BloodSprayList）。
class BloodSpray{
public:
	static const int kMaxDropNumber = 24; // 1回分の滴の数の上限（部位が千切れたときの切り口の数。scale 2 で 12 粒の 2 倍）

	// point: 当たった位置（HitWorld::Hit の値）。direction: 弾の飛んでいた向き（長さ1）。
	// scale: 量と大きさの倍率（1 で体、頭は大きくする。値は持ち主が決める）。1 以上。
	// random: 滴の向き・速さのばらつきに使う。所有権は受け取らない。nullptr 不可。
	BloodSpray( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& direction, float scale, std::mt19937* random );
	void update();            // 1ティック進める。終わった物に呼んでも何もしない
	bool isFinished() const;  // 決まった時間がたったか（持ち主が消す）

	int dropNumber() const;   // 滴の数
	const GameLib::Math::Vector3& dropPosition( int index ) const; // 滴 index（0〜dropNumber-1）の今の位置（ワールド）
	float dropRadius() const;  // 今の板の幅の半分（m。どの滴も同じ）
	float dropOpacity() const; // 今の不透明度（0〜1。どの滴も同じ）

private:
	float progress() const; // 出てから消えるまでの進み具合（0〜1）

	GameLib::Math::Vector3 mPosition[ kMaxDropNumber ]; // 滴の位置（ワールド）
	GameLib::Math::Vector3 mVelocity[ kMaxDropNumber ]; // 滴の速さ（m/ティック）
	int mDropNumber;
	float mScale;
	int mAge; // 出てからのティック数
};

#endif
