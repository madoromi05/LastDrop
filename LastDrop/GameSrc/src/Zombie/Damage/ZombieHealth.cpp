#include "GameSrc/Zombie/Damage/ZombieHealth.h"

#include "GameLib/GameLib.h"

namespace{
	// 全体の体力（丈夫さ 1 のとき）。胴（倍率 1）だけを撃つと 6 発で 0 になる（弾1発 20。Bullet.cpp）。
	const float kMaxTotal = 120.f;

	// 部位ごとの決まり。
	struct PartRule{
		float maxHealth;  // 部位の体力
		float multiplier; // 当たったときのダメージの倍率
		bool fatal;       // この部位が 0 になったら倒れるか
		// 当たるたびに、この確率でその部位の体力を一気に 0 にする（クリティカル。0 なら起きない）。
		// SummaryPlan/個別仕様/ゾンビ/ZombieUncertainty_Plan.txt の 4 章。
		float criticalChance;
	};
	// BodyPart の並びと同じ順（下の static_assert で数を確かめる）。値の理由は SummaryPlan/個別仕様/ゾンビ/ZombieDamage_Plan.txt の 3 章。
	const PartRule kPartRules[] = {
		{  80.f, 1.5f,  true,  0.1f }, // Head          一番効くが、ふだんは 1 発では倒れない（3 発で 0）。10% で弾けて倒れる
		{ 120.f, 1.0f,  false, 0.f  }, // Torso         基準。全体と同じ値なので、胴だけ撃てば全体と同時に 0
		{  40.f, 0.5f,  false, 0.f  }, // LeftUpperArm  腕・すねは「外した」に近い扱い
		{  40.f, 0.5f,  false, 0.f  }, // RightUpperArm
		{  40.f, 0.5f,  false, 0.f  }, // LeftForeArm
		{  40.f, 0.5f,  false, 0.f  }, // RightForeArm
		{  60.f, 0.75f, false, 0.f  }, // LeftThigh     脚は胴より弱いが、腕よりは効く
		{  60.f, 0.75f, false, 0.f  }, // RightThigh
		{  40.f, 0.5f,  false, 0.f  }, // LeftLowerLeg
		{  40.f, 0.5f,  false, 0.f  }, // RightLowerLeg
	};
	static_assert( sizeof( kPartRules ) / sizeof( kPartRules[ 0 ] ) == static_cast< size_t >( BodyPart::Count ),
		"kPartRules must have one entry per BodyPart." );

	const PartRule& ruleOf( BodyPart part ) {
		int index = static_cast< int >( part );
		ASSERT( index >= 0 && index < static_cast< int >( BodyPart::Count ) && "ZombieHealth : bad part." );
		return kPartRules[ index ];
	}

	// value から amount を引く。0 より下へは行かない。
	float subtractToZero( float value, float amount ) {
		float result = value - amount;
		return ( result > 0.f ) ? result : 0.f;
	}
}

ZombieHealth::ZombieHealth( float toughness ) :
	mToughness( toughness ),
	mTotal( kMaxTotal * toughness ) {
	ASSERT( toughness > 0.f && "ZombieHealth : toughness must be positive." );
	for ( int i = 0; i < static_cast< int >( BodyPart::Count ); ++i ) {
		mParts[ i ] = kPartRules[ i ].maxHealth * toughness;
	}
}

void ZombieHealth::takeDamage( BodyPart part, float baseAmount, float criticalRoll ) {
	const PartRule& rule = ruleOf( part );
	float amount = baseAmount * rule.multiplier;
	int index = static_cast< int >( part );
	// クリティカルは部位だけを 0 にする（全体は 1 発ぶんだけ減る）。頭は 0 で倒れる部位なので、それで倒れる。
	if ( criticalRoll < rule.criticalChance ) {
		mParts[ index ] = 0.f;
	} else {
		mParts[ index ] = subtractToZero( mParts[ index ], amount );
	}
	// 壊れた部位に当たっても全体は減り続ける（腕を壊した後も、腕を撃ち続ければいつか倒れる）。
	mTotal = subtractToZero( mTotal, amount );
}

bool ZombieHealth::isDead() const {
	if ( mTotal <= 0.f ) {
		return true;
	}
	for ( int i = 0; i < static_cast< int >( BodyPart::Count ); ++i ) {
		if ( kPartRules[ i ].fatal && mParts[ i ] <= 0.f ) {
			return true;
		}
	}
	return false;
}

float ZombieHealth::total() const {
	return mTotal;
}

float ZombieHealth::maxTotal() const {
	return kMaxTotal * mToughness;
}

float ZombieHealth::part( BodyPart part ) const {
	ruleOf( part ); // 範囲の確認
	return mParts[ static_cast< int >( part ) ];
}

float ZombieHealth::maxPart( BodyPart part ) const {
	return ruleOf( part ).maxHealth * mToughness;
}

bool ZombieHealth::isBroken( BodyPart part ) const {
	return this->part( part ) <= 0.f;
}

float ZombieHealth::weakestLegRatio() const {
	// 部位ごとに見る（太ももとすねを足して 1 本とみると、半分を切る前に太ももが 0 になって切れてしまう。
	// 太もも 60・すね 40 で、1 発は太もも 15・すね 10 なので、部位ごとなら半分で 2 発・0 で 4 発になる）。
	const BodyPart kLegParts[] = { BodyPart::LeftThigh, BodyPart::RightThigh, BodyPart::LeftLowerLeg, BodyPart::RightLowerLeg };
	float weakest = 1.f;
	for ( BodyPart leg : kLegParts ) {
		float ratio = part( leg ) / maxPart( leg );
		weakest = ( ratio < weakest ) ? ratio : weakest;
	}
	return weakest;
}

float ZombieHealth::toughness() const {
	return mToughness;
}
