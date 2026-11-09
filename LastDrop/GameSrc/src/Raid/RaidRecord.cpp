#include "GameSrc/Raid/RaidRecord.h"
#include "GameSrc/Raid/RaidStart.h"

RaidRecord::RaidRecord() :
	mHasForcedCondition( false ),
	mForcedCondition( DayCondition::Normal ) {
}

const RaidResult& RaidRecord::lastResult() const {
	return mLastResult;
}

void RaidRecord::setLastResult( const RaidResult& result ) {
	mLastResult = result;
}

DayCondition RaidRecord::chooseCondition( unsigned seed ) const {
	return mHasForcedCondition ? mForcedCondition : RaidStart::rollCondition( seed );
}

bool RaidRecord::hasForcedCondition() const {
	return mHasForcedCondition;
}

DayCondition RaidRecord::forcedCondition() const {
	return mForcedCondition;
}

void RaidRecord::forceCondition( DayCondition condition ) {
	mHasForcedCondition = true;
	mForcedCondition = condition;
}

void RaidRecord::clearForcedCondition() {
	mHasForcedCondition = false;
}
