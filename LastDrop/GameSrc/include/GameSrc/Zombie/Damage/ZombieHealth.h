#ifndef INCLUDED_ZOMBIE_HEALTH_H
#define INCLUDED_ZOMBIE_HEALTH_H

#include "GameSrc/Hit/BodyPart.h"

// ゾンビ1体の体力。全体の体力と、部位ごとの体力を持つ。
// ・全体の体力：生きているかを決める。0 になったら倒れる
// ・部位の体力：その部位がどれだけ傷んだか。「0 で倒れる部位」（頭）が 0 になっても倒れる。
//   ほかの部位は 0 で「壊れた」になるだけ（後で部位欠損・脚を失って這う、をここから決める）
// 1発のダメージは「倍率を掛ける前の量 × 当たった部位の倍率」で、部位と全体の両方から引く。どちらも 0 で止まる。
// 部位ごとの最大の体力・倍率・0 で倒れるかの表は、この .cpp だけが知っている。
// 体ごとの丈夫さ（toughness）を、全体と全部の部位の最大に同じだけ掛ける（部位どうしの比は表のまま。
// SummaryPlan/個別仕様/ゾンビ/ZombieUncertainty_Plan.txt の 3 章）。丈夫さを選ぶのは Zombie（ここは乱数を持たない）。
// 倒れた後どうするか（アニメ・名簿から外す）は知らない（Zombie が isDead を見て決める）。
class ZombieHealth{
public:
	// 全部満タンで始める。toughness: 最大の体力に掛ける値（1 で表のまま）。0 より大きいこと。
	explicit ZombieHealth( float toughness );

	// part に baseAmount（倍率を掛ける前の量）のダメージを受ける。
	// criticalRoll: 0 以上 1 未満の乱数。部位のクリティカルの確率（表。頭だけ 10%）より小さければ、その部位の体力を一気に 0 にする
	// （頭が弾ける。SummaryPlan/個別仕様/ゾンビ/ZombieUncertainty_Plan.txt の 4 章）。乱数を引くのは呼ぶ側（ここは乱数を持たない）。
	void takeDamage( BodyPart part, float baseAmount, float criticalRoll );

	// 倒れる条件を満たしたか（全体が 0、または 0 で倒れる部位が 0）。
	bool isDead() const;
	float total() const;    // 全体の残り（0 以上）
	float maxTotal() const; // 全体の最大（丈夫さを掛けた後）
	float part( BodyPart part ) const;    // 部位の残り（0 以上）
	float maxPart( BodyPart part ) const; // 部位の最大（丈夫さを掛けた後）
	// 部位の体力が 0 になったか（0 で倒れる部位も、0 なら true）。
	bool isBroken( BodyPart part ) const;
	// 脚の 4 部位（左右の太もも・すね）のうち、一番傷んだ部位の残りの割合（0〜1。満タンで 1）。
	// 走る体が歩きに落ちるかを決めるのに使う（R4_Plan.txt Q7）。
	float weakestLegRatio() const;
	float toughness() const; // 最大の体力に掛けている値（確認用の表示に使う）

private:
	float mToughness;
	float mTotal;
	float mParts[ static_cast< int >( BodyPart::Count ) ]; // BodyPart の番号で引く
};

#endif
