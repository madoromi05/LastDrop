#include "GameSrc/Zombie/ZombieHealth.h"

#include "GameLib/GameLib.h"

namespace{
	// 全体の体力。胴（倍率 1）だけを撃つと 3 発で 0 になる（7.62 NATO 1発 40）。
	const float kMaxTotal = 120.f;

	// 部位ごとの決まり。
	struct PartRule{
		float maxHealth;  // 部位の体力
		float multiplier; // 当たったときのダメージの倍率
		bool fatal;       // この部位が 0 になったら倒れるか
	};
	// BodyPart の並びと同じ順（下の static_assert で数を確かめる）。値の理由は SummaryPlan/個別仕様/ZombieDamage_Plan.txt の 3 章。
	const PartRule kPartRules[] = {
		{  80.f, 1.5f,  true  }, // Head          一番効くが 1 発では倒れない（2 発で 0）
		{ 120.f, 1.0f,  false }, // Torso         基準。全体と同じ値なので、胴だけ撃てば全体と同時に 0
		{  40.f, 0.5f,  false }, // LeftUpperArm  腕・すねは「外した」に近い扱い
		{  40.f, 0.5f,  false }, // RightUpperArm
		{  40.f, 0.5f,  false }, // LeftForeArm
		{  40.f, 0.5f,  false }, // RightForeArm
		{  60.f, 0.75f, false }, // LeftThigh     脚は胴より弱いが、腕よりは効く
		{  60.f, 0.75f, false }, // RightThigh
		{  40.f, 0.5f,  false }, // LeftLowerLeg
		{  40.f, 0.5f,  false }, // RightLowerLeg
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

ZombieHealth::ZombieHealth() :
	mTotal( kMaxTotal ) {
	for ( int i = 0; i < static_cast< int >( BodyPart::Count ); ++i ) {
		mParts[ i ] = kPartRules[ i ].maxHealth;
	}
}

void ZombieHealth::takeDamage( BodyPart part, float baseAmount ) {
	float amount = baseAmount * ruleOf( part ).multiplier;
	int index = static_cast< int >( part );
	mParts[ index ] = subtractToZero( mParts[ index ], amount );
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
	return kMaxTotal;
}

float ZombieHealth::part( BodyPart part ) const {
	ruleOf( part ); // 範囲の確認
	return mParts[ static_cast< int >( part ) ];
}

float ZombieHealth::maxPart( BodyPart part ) const {
	return ruleOf( part ).maxHealth;
}

bool ZombieHealth::isBroken( BodyPart part ) const {
	return this->part( part ) <= 0.f;
}
