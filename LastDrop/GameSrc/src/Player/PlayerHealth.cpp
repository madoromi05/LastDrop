#include "GameSrc/Player/PlayerHealth.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;

namespace{
	const int kZoneNumber = static_cast< int >( PlayerBodyZone::Count );

	// 部位ごとの決まりごと。並びは PlayerBodyZone と同じ（static_assert で数を確かめる）。
	// 体力はタルコフと同じ値（計 440）。2026-10-20 ユーザー指定。
	struct ZoneSpec{
		float maxHealth;
		bool fatal; // 0 になったら死ぬか
	};
	const bool kFatal = true;
	const bool kBreaks = false;
	const ZoneSpec kZoneSpecs[] = {
		{ 35.f, kFatal  }, // Head
		{ 85.f, kFatal  }, // Thorax
		{ 70.f, kBreaks }, // Stomach
		{ 60.f, kBreaks }, // LeftArm
		{ 60.f, kBreaks }, // RightArm
		{ 65.f, kBreaks }, // LeftLeg
		{ 65.f, kBreaks }, // RightLeg
	};
	static_assert( sizeof( kZoneSpecs ) / sizeof( kZoneSpecs[ 0 ] ) == kZoneNumber, "kZoneSpecs must cover every PlayerBodyZone." );

	// 壊れた部位へのダメージを、ほかの部位へ散らすときに掛ける割合（タルコフの目安に合わせた初期値）。
	const float kSpreadScale = 0.7f;
	// 脚が壊れたときの歩く速さの倍率。
	const float kBrokenLegSpeedScale = 0.75f;
	const float kFullSpeedScale = 1.f;

	// 噛まれる所と、その割合（足して 1）。前からつかんで、首・肩（胸）と、払おうとする腕に噛みつく。
	// 頭・腹・脚へは直接入らない（散った分で減ることはある）。
	struct BiteChance{
		PlayerBodyZone zone;
		float chance;
	};
	const BiteChance kBiteChances[] = {
		{ PlayerBodyZone::Thorax,   0.4f },
		{ PlayerBodyZone::LeftArm,  0.3f },
		{ PlayerBodyZone::RightArm, 0.3f },
	};

	const ZoneSpec& spec( PlayerBodyZone zone ) {
		return kZoneSpecs[ static_cast< int >( zone ) ];
	}

	PlayerBodyZone zoneAt( int index ) {
		return static_cast< PlayerBodyZone >( index );
	}
}

PlayerHealth::PlayerHealth() {
	for ( int i = 0; i < kZoneNumber; ++i ) {
		mParts[ i ] = kZoneSpecs[ i ].maxHealth;
	}
}

void PlayerHealth::takeDamage( PlayerBodyZone zone, float amount ) {
	if ( isDead() ) {
		return;
	}
	// 壊れていればそのまま全部、壊れていなければ 0 を超えた分だけを散らす（どちらも「壊れた部位へのダメージ」）。
	float overflow = subtract( zone, amount );
	if ( overflow > 0.f && !isDead() ) {
		spread( overflow );
	}
}

float PlayerHealth::subtract( PlayerBodyZone zone, float amount ) {
	float& health = mParts[ static_cast< int >( zone ) ];
	float taken = Math::min( health, amount );
	health -= taken;
	return amount - taken;
}

void PlayerHealth::spread( float amount ) {
	int receivers = 0;
	for ( int i = 0; i < kZoneNumber; ++i ) {
		if ( !isBroken( zoneAt( i ) ) ) {
			++receivers;
		}
	}
	if ( receivers == 0 ) {
		return;
	}
	// 散った先でさらに 0 を超えた分は、もう散らさない（散らす先を数え直して回り続けないように。1 回で決める）。
	float share = amount * kSpreadScale / static_cast< float >( receivers );
	for ( int i = 0; i < kZoneNumber; ++i ) {
		if ( !isBroken( zoneAt( i ) ) ) {
			subtract( zoneAt( i ), share );
		}
	}
}

PlayerBodyZone PlayerHealth::biteZone( float roll ) {
	float accumulated = 0.f;
	for ( const BiteChance& entry : kBiteChances ) {
		accumulated += entry.chance;
		if ( roll < accumulated ) {
			return entry.zone;
		}
	}
	// 割合の足し算の誤差で 1 に届かなかったときは、最後の所。
	const int kLast = sizeof( kBiteChances ) / sizeof( kBiteChances[ 0 ] ) - 1;
	return kBiteChances[ kLast ].zone;
}

bool PlayerHealth::isDead() const {
	for ( int i = 0; i < kZoneNumber; ++i ) {
		if ( kZoneSpecs[ i ].fatal && mParts[ i ] <= 0.f ) {
			return true;
		}
	}
	return false;
}

float PlayerHealth::part( PlayerBodyZone zone ) const {
	return mParts[ static_cast< int >( zone ) ];
}

float PlayerHealth::maxPart( PlayerBodyZone zone ) const {
	return spec( zone ).maxHealth;
}

bool PlayerHealth::isBroken( PlayerBodyZone zone ) const {
	return part( zone ) <= 0.f;
}

bool PlayerHealth::isLegBroken() const {
	return isBroken( PlayerBodyZone::LeftLeg ) || isBroken( PlayerBodyZone::RightLeg );
}

bool PlayerHealth::canRun() const {
	return !isLegBroken();
}

float PlayerHealth::walkSpeedScale() const {
	return isLegBroken() ? kBrokenLegSpeedScale : kFullSpeedScale;
}
