#include "GameSrc/Interaction/ItemCatalog.h"

#include "GameLib/GameLib.h"

namespace{
	// ItemId の並びと同じ順。
	const ItemSpec kItemSpecs[ ITEM_NUMBER ] = {
		{ "Fuel", 1, 0 }, // ITEM_FUEL: 1缶ずつしか運べない（車へ入れてから次を拾う）
		// ITEM_AMMO: 始めは弾倉 2 本ぶん（弾倉の 30 発とは別）。持てるのは 4 本ぶんまで（拾い続けても弾が余りすぎない。バイオハザードの乏しさ）。
		{ "7.62 rounds", 120, 60 },
	};
}

const ItemSpec& itemSpec( ItemId id ) {
	ASSERT( id >= 0 && id < ITEM_NUMBER && "itemSpec : bad ItemId." );
	return kItemSpecs[ id ];
}
