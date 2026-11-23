#include "GameSrc/DebugMode/TownEditReload.h"

#ifdef ZF_EDITOR

#include "GameSrc/Raid/RaidRecord.h"
#include "GameSrc/Map/TownMap.h"

#include "GameLib/GameLib.h"

TownEditReload::TownEditReload() :
	mPending( false ),
	mCarSpot( TownMap::kNoEscapeCarSpot ) {	// まだ何も頼んでいない
}

void TownEditReload::begin( RaidRecord* record, DayCondition condition, int carSpot ) {
	ASSERT( record && !mPending && "TownEditReload::begin : no record, or already waiting for the reload." );
	record->overrideNextRaid( condition, carSpot );
	mCarSpot = carSpot;
	mPending = true;
}

bool TownEditReload::isPending() const {
	return mPending;
}

TownEditReload::Result TownEditReload::poll( const RaidRecord& record, bool raidLoaded, int loadedSpot ) {
	if ( !mPending ) {
		return Result::Idle;
	}
	if ( record.hasNextRaidOverride() || !raidLoaded ) {	// 決め事が残っていれば、見えている世界は前の出撃。使われた後も、世界が渡るまではロード中
		return Result::Waiting;
	}
	mPending = false;
	return ( loadedSpot == mCarSpot ) ? Result::Loaded : Result::Replaced;
}

#endif // ZF_EDITOR
