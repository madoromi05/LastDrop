#include "GameSrc/Raid/RaidStart.h"
#include "GameSrc/Weapon/WeaponCatalog.h"

#include "GameLib/GameLib.h"
#include "GameLib/Math/Random.h"

namespace{
	// 始めのカバンの物（銃のほか）
	struct StartItem{
		ItemId item;
		int amount;
	};
	const StartItem kStartItems[] = {
		{ ITEM_AMMO_9MM, 24 },
		{ ITEM_AMMO_9MM, 24 },
	};
	// 始めのカバンの銃（弾倉いっぱい）
	const WeaponId kStartWeapon = WEAPON_PISTOL;

	// 選ぶ条件の表。ふつうを外す・群れを出にくくする（2 行入れると 2 倍）ときは、この表だけを直す
	const DayCondition kConditionChoices[] = {
		DayCondition::Normal,
		DayCondition::Blackout,
		DayCondition::Fog,
		DayCondition::Rain,
		DayCondition::Horde,
	};
	const int kConditionChoiceNumber = sizeof( kConditionChoices ) / sizeof( kConditionChoices[ 0 ] );
	// 種を混ぜる数。車の場所・ガソリンの箱も同じ種（起動からの時間）で引くので、そのまま使うと条件と車の場所がそろって出てしまう
	const unsigned kConditionSeedMix = 0x5BD1E995u;
	// Random は 0 以下の種を決まった種に置き換える（毎回同じ条件になる）ので、上の 1 ビットを落として正にする
	const unsigned kPositiveMask = 0x7FFFFFFFu;
	const int kFallbackSeed = 1; // 混ぜた結果がちょうど 0 になったとき
}

namespace RaidStart{

ItemGrid startBag() {
	ItemGrid bag( kBagColumns, kBagRows );
	const WeaponSpec& weapon = weaponSpec( kStartWeapon );
	ItemStack gun = { weapon.item, 1 };
	gun.loadedRounds = weapon.magazineCapacity;
	bool placed = bag.placeAnywhere( gun );
	for ( const StartItem& start : kStartItems ) {
		ItemStack stack = { start.item, start.amount };
		placed = bag.placeAnywhere( stack ) && placed;
	}
	ASSERT( placed && "RaidStart::startBag : the start items do not fit in the bag." );
	return bag;
}

DayCondition rollCondition( unsigned seed ) {
	int mixed = static_cast< int >( ( seed ^ kConditionSeedMix ) & kPositiveMask );
	GameLib::Math::Random random = GameLib::Math::Random::create( mixed > 0 ? mixed : kFallbackSeed );
	return kConditionChoices[ random.getInt( kConditionChoiceNumber ) ];
}

} //namespace RaidStart
