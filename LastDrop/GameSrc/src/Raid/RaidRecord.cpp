#include "GameSrc/Raid/RaidRecord.h"
#include "GameSrc/Raid/RaidStart.h"

#include "GameLib/GameLib.h"

namespace{
	const int kRandomCarSpot = -1;	// 車の置き場所を決めていない（負なら RaidSession が選ぶ）
}

RaidRecord::RaidRecord() :
	mHasForcedCondition( false ),
	mForcedCondition( DayCondition::Normal ),
	mHasNextRaidOverride( false ),
	mNextRaidOverride{ DayCondition::Normal, kRandomCarSpot } {
}

const RaidResult& RaidRecord::lastResult() const {
	return mLastResult;
}

void RaidRecord::setLastResult( const RaidResult& result ) {
	mLastResult = result;
}

RaidRecord::RaidChoice RaidRecord::takeRaidChoice( unsigned seed ) {
	if ( mHasNextRaidOverride ) {
		mHasNextRaidOverride = false;	// 決め事は 1 回だけ。次の出撃は固定かランダムに戻る
		return mNextRaidOverride;
	}
	const DayCondition condition = mHasForcedCondition ? mForcedCondition : RaidStart::rollCondition( seed );
	return { condition, kRandomCarSpot };
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

void RaidRecord::overrideNextRaid( DayCondition condition, int carSpot ) {
	ASSERT( carSpot >= 0 && "RaidRecord::overrideNextRaid : carSpot must not be negative." );
	mHasNextRaidOverride = true;
	mNextRaidOverride = { condition, carSpot };
}

bool RaidRecord::hasNextRaidOverride() const {
	return mHasNextRaidOverride;
}

void RaidRecord::dropNextRaidOverride() {
	mHasNextRaidOverride = false;
}
