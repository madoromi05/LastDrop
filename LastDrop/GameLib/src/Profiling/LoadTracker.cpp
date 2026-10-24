#ifdef ZF_EDITOR

#include "Profiling/LoadTracker.h"
#include "Profiling/FrameTimeline.h"
#include "Profiling/JsonWriter.h"
#include "Profiling/OutputFiles.h"
#include "Profiling/SystemInfo.h"
#include "GameLib/Profiling/Profiler.h"

#include <algorithm>
#include <cstdlib>

namespace GameLib{
namespace Profiling{

namespace{
	// スパイクの JSON のために残す、直近の読み込みの件数。
	const size_t kRecentItemLimit = 256;
	const char* const kWaitCategory = "wait";
	const char* const kLaunchCountFile = "launch_count.txt";
	const double kMillisecondsPerSecond = 1000.0;
	const int kFormatVersion = 1; // JSON の形を変えたら上げる

	bool longerFirst( const LoadItem& a, const LoadItem& b ){
		return a.seconds > b.seconds;
	}
}

LoadTracker::LoadTracker() :
mActive( false ),
mEndRequested( false ),
mName( "" ),
mBegin( 0 ),
mFrames( 0 ),
mLongestFrameSeconds( 0.0 ),
mLaunch( countLaunch() ){
	mLast.attempt = 0;
	mLast.seconds = 0.0;
}

void LoadTracker::begin( const char* name, long long at ){
	if ( mActive ){
		return;
	}
	mActive = true;
	mEndRequested = false;
	mName = name;
	mBegin = at;
	mFrames = 0;
	mLongestFrameSeconds = 0.0;
	mItems.clear();
}

void LoadTracker::requestEnd(){
	if ( mActive ){
		mEndRequested = true;
	}
}

bool LoadTracker::isLoading() const {
	return mActive;
}

void LoadTracker::addItem( const char* category, const char* name, double seconds, long long frame ){
	LoadItem item;
	item.category = category;
	item.name = name ? name : "";
	item.seconds = seconds;
	item.frame = frame;
	if ( mActive ){
		mItems.push_back( item );
	}
	mRecent.push_back( item );
	if ( mRecent.size() > kRecentItemLimit ){
		mRecent.pop_front();
	}
}

bool LoadTracker::onPresented( const FrameRecord& frame ){
	if ( !mActive ){
		return false;
	}
	++mFrames;
	mLongestFrameSeconds = std::max( mLongestFrameSeconds, secondsBetween( frame.begin, frame.presentEnd ) );
	if ( !mEndRequested ){
		return false;
	}
	finish( frame.presentEnd );
	return true;
}

const std::deque< LoadItem >& LoadTracker::recentItems() const {
	return mRecent;
}

const LoadTracker::Summary& LoadTracker::lastSummary() const {
	return mLast;
}

void LoadTracker::finish( long long at ){
	mActive = false;
	mEndRequested = false;
	int attempt = ++mAttempts[ mName ];
	double total = secondsBetween( mBegin, at );
	mLast.name = mName;
	mLast.attempt = attempt;
	mLast.seconds = total;

	// メインスレッドの処理は種類ごとに足す。待ちは並んで進むので足さず、一番長いものだけを出す。
	std::map< std::string, double > mainThreadSeconds;
	double longestWait = 0.0;
	for ( const LoadItem& item : mItems ){
		if ( item.category == kWaitCategory ){
			longestWait = std::max( longestWait, item.seconds );
		}else{
			mainThreadSeconds[ item.category ] += item.seconds;
		}
	}
	std::sort( mItems.begin(), mItems.end(), longerFirst );
	mLast.slowest.clear();
	for ( const LoadItem& item : mItems ){
		if ( static_cast< int >( mLast.slowest.size() ) >= Summary::kSlowestCount ){
			break;
		}
		if ( item.category != kWaitCategory ){
			mLast.slowest.push_back( item );
		}
	}

	JsonWriter json;
	json.beginObject( nullptr );
	json.integer( "version", kFormatVersion );
	json.string( "name", mName );
	json.integer( "attempt", attempt );
	json.integer( "launch", mLaunch );
	json.number( "totalMs", total * kMillisecondsPerSecond );
	json.integer( "frames", mFrames );
	json.number( "longestFrameMs", mLongestFrameSeconds * kMillisecondsPerSecond );
	json.number( "longestWaitMs", longestWait * kMillisecondsPerSecond );
	json.beginObject( "mainThreadMsByCategory" );
	for ( const auto& pair : mainThreadSeconds ){
		json.number( pair.first.c_str(), pair.second * kMillisecondsPerSecond );
	}
	json.endObject();
	json.beginArray( "items" );
	for ( const LoadItem& item : mItems ){
		json.beginObject( nullptr );
		json.string( "category", item.category );
		json.string( "name", item.name );
		json.number( "ms", item.seconds * kMillisecondsPerSecond );
		json.endObject();
	}
	json.endArray();
	SystemInfo::write( "system", &json );
	json.endObject();

	std::string fileName = "load_" + std::string( mName ) + "_" + OutputFiles::timestamp() + ".json";
	OutputFiles::write( OutputFiles::pathOf( fileName ), json.text() );
	mItems.clear();
}

int LoadTracker::countLaunch(){
	std::string path = OutputFiles::pathOf( kLaunchCountFile );
	std::string text;
	int launch = 1;
	if ( OutputFiles::read( path, &text ) ){
		launch = atoi( text.c_str() ) + 1;
	}
	OutputFiles::write( path, std::to_string( launch ) );
	return launch;
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
