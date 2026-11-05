#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Map/SpawnPicker.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

namespace{
	const int kOneCar = 1; // 脱出の車は毎日 1 台（FuelEscapeLoading_Plan.txt Q1）
}

RaidSession::RaidSession( const std::vector< TownEscapeCarSpot >& carSpots, unsigned seed ) :
	mEscapeCarSpot( pickCarSpot( carSpots, seed ) ),
	mClock( raidParams().raidSeconds ),
	mExtraction( zoneAt( carSpots, mEscapeCarSpot ), raidParams().extractRadius, raidParams().extractSeconds ),
	mKills( 0 ),
	mEnding( RaidEnding::None ) {
}

int RaidSession::pickCarSpot( const std::vector< TownEscapeCarSpot >& carSpots, unsigned seed ) {
	ASSERT( !carSpots.empty() && "RaidSession : no escape car spot." );
	return SpawnPicker::pick( static_cast< int >( carSpots.size() ), kOneCar, seed ).front();
}

std::vector< ExtractZone > RaidSession::zoneAt( const std::vector< TownEscapeCarSpot >& carSpots, int spot ) {
	const TownPlacement& p = carSpots[ spot ].placement;
	return { { carSpots[ spot ].name, Vector3( p.x, p.y, p.z ) } };
}

int RaidSession::escapeCarSpot() const {
	return mEscapeCarSpot;
}

void RaidSession::openEscape() {
	mExtraction.open();
}

void RaidSession::tick( const Vector3& feet, bool playerDead, int kills ) {
	if ( mEnding != RaidEnding::None ) {
		return;
	}
	mKills += kills;
	if ( playerDead ) {
		mEnding = RaidEnding::Killed;
		return;
	}
	mClock.tick();
	mExtraction.tick( feet );
	if ( mExtraction.hasExtracted() ) {
		mEnding = RaidEnding::Survived;
	} else if ( mClock.isExpired() ) {
		mEnding = RaidEnding::Missing;
	}
}

RaidEnding RaidSession::ending() const {
	return mEnding;
}

const RaidClock& RaidSession::clock() const {
	return mClock;
}

const ExtractionTracker& RaidSession::extraction() const {
	return mExtraction;
}

int RaidSession::kills() const {
	return mKills;
}

DayResult RaidSession::result() const {
	DayResult result;
	result.ending = mEnding;
	result.seconds = mClock.elapsedSeconds();
	result.kills = mKills;
	return result;
}

void RaidSession::setRemainingSeconds( float seconds ) {
	mClock.setRemainingSeconds( seconds );
}

void RaidSession::forceExtract() {
	mExtraction.forceExtract();
}
