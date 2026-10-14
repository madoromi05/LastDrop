#include "GameSrc/Interaction/ItemCatalog.h"

#include "GameLib/GameLib.h"

namespace{
	// ItemId の並びと同じ順。
	const ItemSpec kItemSpecs[ ITEM_NUMBER ] = {
		{ "Fuel", 1 }, // ITEM_FUEL: 1缶ずつしか運べない（車へ入れてから次を拾う）
	};
}

const ItemSpec& itemSpec( ItemId id ) {
	ASSERT( id >= 0 && id < ITEM_NUMBER && "itemSpec : bad ItemId." );
	return kItemSpecs[ id ];
}
