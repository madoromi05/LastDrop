#include "GameSrc/Raid/ExtractionTracker.h"
#include "GameSrc/Core/GameTime.h"

#include "GameLib/GameLib.h"

using namespace GameLib::Math;

ExtractionTracker::ExtractionTracker( const std::vector< ExtractZone >& zones, float radius, float seconds ) :
	mZones( zones ),
	mRadius( radius ),
	mRequiredTicks( GameTime::secondsToTicks( seconds ) ),
	mInsideTicks( 0 ),
	mInside( false ),
	mExtracted( false ) {
	ASSERT( radius > 0.f && mRequiredTicks > 0 && "ExtractionTracker : radius and seconds must be positive." );
}

void ExtractionTracker::tick( const Vector3& feet ) {
	if ( mExtracted ) {
		return;
	}
	mInside = false;
	for ( const ExtractZone& zone : mZones ) {
		// 高さは見ない（歩道の段差・車道の面のどちらでも同じ円に入る）。
		Vector3 toFeet;
		toFeet.setSub( feet, zone.position );
		toFeet.y = 0.f;
		if ( toFeet.squareLength() <= mRadius * mRadius ) {
			mInside = true;
			break;
		}
	}
	mInsideTicks = mInside ? mInsideTicks + 1 : 0;
	if ( mInsideTicks >= mRequiredTicks ) {
		mExtracted = true;
	}
}

bool ExtractionTracker::isInside() const {
	return mInside;
}

float ExtractionTracker::remainingSeconds() const {
	int remaining = mRequiredTicks - mInsideTicks;
	return GameTime::ticksToSeconds( remaining > 0 ? remaining : 0 );
}

bool ExtractionTracker::hasExtracted() const {
	return mExtracted;
}

void ExtractionTracker::forceExtract() {
	mExtracted = true;
}

const std::vector< ExtractZone >& ExtractionTracker::zones() const {
	return mZones;
}
