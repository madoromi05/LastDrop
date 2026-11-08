#include "GameSrc/Raid/RunState.h"

#include "GameLib/GameLib.h"
#include <algorithm>
#include <iterator>

RunState::RunState() :
	mDay( kFirstDay ),
	mStash( kStashColumns, kStashRows ),
	mBag( kBagColumns, kBagRows ),
	mKills( 0 ),
	mBestScore( 0 ),
	mRunEndPending( false ) {
	std::fill( std::begin( mConditions ), std::end( mConditions ), DayCondition::Normal );
}

int RunState::day() const {
	return mDay;
}

ItemGrid& RunState::stash() {
	return mStash;
}

const ItemGrid& RunState::stash() const {
	return mStash;
}

ItemGrid& RunState::bag() {
	return mBag;
}

const ItemGrid& RunState::bag() const {
	return mBag;
}

int RunState::kills() const {
	return mKills;
}

void RunState::setDay( int day ) {
	ASSERT( day >= kFirstDay && "RunState::setDay : bad day." );
	mDay = day;
}

void RunState::setKills( int kills ) {
	mKills = kills;
}

DayCondition RunState::condition( int day ) const {
	ASSERT( day >= kFirstDay && day <= kDayCount && "RunState::condition : bad day." );
	return mConditions[ day - kFirstDay ];
}

void RunState::setCondition( int day, DayCondition condition ) {
	ASSERT( day >= kFirstDay && day <= kDayCount && "RunState::setCondition : bad day." );
	mConditions[ day - kFirstDay ] = condition;
}

int RunState::bestScore() const {
	return mBestScore;
}

void RunState::setBestScore( int score ) {
	mBestScore = score;
}

const DayResult& RunState::lastDay() const {
	return mLastDay;
}

void RunState::setLastDay( const DayResult& result ) {
	mLastDay = result;
}

const RunEnd& RunState::lastRunEnd() const {
	return mLastRunEnd;
}

void RunState::setLastRunEnd( const RunEnd& end ) {
	mLastRunEnd = end;
}

bool RunState::isRunEndPending() const {
	return mRunEndPending;
}

void RunState::setRunEndPending( bool pending ) {
	mRunEndPending = pending;
}

