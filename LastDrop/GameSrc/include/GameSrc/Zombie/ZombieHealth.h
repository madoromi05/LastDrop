#ifndef INCLUDED_ZOMBIE_HEALTH_H
#define INCLUDED_ZOMBIE_HEALTH_H

#include "GameSrc/Hit/BodyPart.h"

// ゾンビ1体の体力。全体の体力と、部位ごとの体力を持つ。
// ・全体の体力：生きているかを決める。0 になったら倒れる
// ・部位の体力：その部位がどれだけ傷んだか。「0 で倒れる部位」（頭）が 0 になっても倒れる。
//   ほかの部位は 0 で「壊れた」になるだけ（後で部位欠損・脚を失って這う、をここから決める）
// 1発のダメージは「倍率を掛ける前の量 × 当たった部位の倍率」で、部位と全体の両方から引く。どちらも 0 で止まる。
// 部位ごとの最大の体力・倍率・0 で倒れるかの表は、この .cpp だけが知っている。
// 倒れた後どうするか（アニメ・名簿から外す）は知らない（Zombie が isDead を見て決める）。
class ZombieHealth{
public:
	ZombieHealth(); // 全部満タンで始める

	// part に baseAmount（倍率を掛ける前の量）のダメージを受ける。
	void takeDamage( BodyPart part, float baseAmount );

	// 倒れる条件を満たしたか（全体が 0、または 0 で倒れる部位が 0）。
	bool isDead() const;
	float total() const;    // 全体の残り（0 以上）
	float maxTotal() const; // 全体の最大
	float part( BodyPart part ) const;    // 部位の残り（0 以上）
	float maxPart( BodyPart part ) const; // 部位の最大
	// 部位の体力が 0 になったか（0 で倒れる部位も、0 なら true）。
	bool isBroken( BodyPart part ) const;

private:
	float mTotal;
	float mParts[ static_cast< int >( BodyPart::Count ) ]; // BodyPart の番号で引く
};

#endif
