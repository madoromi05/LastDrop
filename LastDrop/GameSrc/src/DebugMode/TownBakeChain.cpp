#include "GameSrc/DebugMode/TownBakeChain.h"

#ifdef ZF_EDITOR

#include "GameSrc/Map/TownMap.h"

TownBakeChain::TownBakeChain() :
	mStartSpot( TownMap::kNoEscapeCarSpot ),
	mReloadSpot( TownMap::kNoEscapeCarSpot ),
	mRunning( false ),
	mReloaded( false ) {
}

void TownBakeChain::start( const std::vector< int >& spots, int startSpot ) {
	mSpots.clear();
	bool bakesStartSpot = false;
	for ( int spot : spots ) {
		if ( spot == startSpot ) {
			bakesStartSpot = true;	// 最後に回す
			continue;
		}
		mSpots.push_back( spot );
	}
	if ( bakesStartSpot ) {
		mSpots.push_back( startSpot );
	}
	mStartSpot = startSpot;
	mReloadSpot = TownMap::kNoEscapeCarSpot;
	mRunning = !mSpots.empty();
	mReloaded = false;
}

bool TownBakeChain::isRunning() const {
	return mRunning;
}

TownBakeChain::Step TownBakeChain::next( int loadedSpot, bool loadedIsCurrent ) {
	if ( !mRunning ) {
		return Step::Idle;
	}
	if ( mSpots.empty() ) {	// 全部書いた。始めの置き場所へ戻るだけなので、手直しと合うかは見ない
		if ( loadedSpot == mStartSpot ) {
			mRunning = false;
			return Step::Done;
		}
		return requestReload( mStartSpot );
	}
	const int spot = mSpots.front();
	if ( loadedSpot != spot ) {
		return requestReload( spot );
	}
	if ( loadedIsCurrent ) {
		return Step::Bake;
	}
	if ( mReloaded ) {	// 読み直しても合わないなら、読み直し続けずに止める
		stop();
		return Step::OutOfDate;
	}
	return requestReload( spot );	// 押したときの出撃が、手直しを変える前に読んだ物だった
}

int TownBakeChain::reloadSpot() const {
	return mReloadSpot;
}

void TownBakeChain::finishBake( bool written ) {
	if ( !mRunning || mSpots.empty() ) {
		return;
	}
	if ( !written ) {
		stop();
		return;
	}
	mSpots.erase( mSpots.begin() );
}

void TownBakeChain::stop() {
	mSpots.clear();
	mRunning = false;
}

TownBakeChain::Step TownBakeChain::requestReload( int spot ) {
	mReloadSpot = spot;
	mReloaded = true;
	return Step::Reload;
}

#endif // ZF_EDITOR
