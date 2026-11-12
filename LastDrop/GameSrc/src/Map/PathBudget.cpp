#include "GameSrc/Map/PathBudget.h"

#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"

#include <algorithm>

PathBudget::PathBudget() :
	mRemaining( 0 ),
	mTick( 0 ) {
}

void PathBudget::beginTick( int searchesPerTick ) {
	mRemaining = searchesPerTick;
	int previousTick = mTick;
	++mTick;
	mQueue.erase( std::remove_if( mQueue.begin(), mQueue.end(),
		[ previousTick ]( const Request& request ) { return request.lastTick < previousTick; } ), mQueue.end() );
	PROFILE_COUNT( "Path.waiting", static_cast< double >( mQueue.size() ) );
}

bool PathBudget::tryAcquire( const void* requester ) {
	ASSERT( requester && "PathBudget : requester is null." );
	auto it = find( requester );
	if ( it == mQueue.end() ) {
		Request request;
		request.requester = requester;
		request.lastTick = mTick;
		it = mQueue.insert( mQueue.end(), request );
	}
	it->lastTick = mTick;
	// 列の前から、残りの回数ぶんだけ通す（前に並ぶ体がこのティックにまだ頼みに来ていなくても、その分を後ろの体に回さない。順番を守るため）
	int position = static_cast< int >( it - mQueue.begin() );
	if ( position >= mRemaining ) {
		return false;
	}
	mQueue.erase( it );
	--mRemaining;
	return true;
}

void PathBudget::cancel( const void* requester ) {
	auto it = find( requester );
	if ( it != mQueue.end() ) {
		mQueue.erase( it );
	}
}

bool PathBudget::isWaiting( const void* requester ) const {
	return find( requester ) != mQueue.end();
}

std::vector< PathBudget::Request >::iterator PathBudget::find( const void* requester ) {
	return std::find_if( mQueue.begin(), mQueue.end(), [ requester ]( const Request& request ) { return request.requester == requester; } );
}

std::vector< PathBudget::Request >::const_iterator PathBudget::find( const void* requester ) const {
	return std::find_if( mQueue.begin(), mQueue.end(), [ requester ]( const Request& request ) { return request.requester == requester; } );
}
