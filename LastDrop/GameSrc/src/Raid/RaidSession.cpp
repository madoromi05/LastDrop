#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Map/SpawnPicker.h"

#include <algorithm>

using namespace GameLib::Math;

namespace{
	// 毎日開く脱出点の数（RaidLoop Q5。4 つのうち 2 つ）。
	const int kOpenExtractCount = 2;
}

RaidSession::RaidSession( const std::vector< TownExtractPoint >& extractPoints, unsigned seed ) :
	mClock( raidParams().raidSeconds ),
	mExtraction( pickOpenZones( extractPoints, seed ), raidParams().extractRadius, raidParams().extractSeconds ),
	mKills( 0 ),
	mEnding( RaidEnding::None ) {
}

std::vector< ExtractZone > RaidSession::pickOpenZones( const std::vector< TownExtractPoint >& points, unsigned seed ) {
	int count = static_cast< int >( points.size() );
	std::vector< int > picked = SpawnPicker::pick( count, std::min( kOpenExtractCount, count ), seed );
	// 画面に出す順は表の順（北・東・南・西）にそろえる（毎日ばらばらの順に並ぶと読みにくい）。
	std::sort( picked.begin(), picked.end() );
	std::vector< ExtractZone > zones;
	for ( int index : picked ) {
		const TownPlacement& p = points[ index ].placement;
		zones.push_back( { points[ index ].name, Vector3( p.x, p.y, p.z ) } );
	}
	return zones;
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
