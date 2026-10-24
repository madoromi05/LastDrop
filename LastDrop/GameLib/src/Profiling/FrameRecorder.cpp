#ifdef ZF_EDITOR

#include "Profiling/FrameRecorder.h"
#include "Profiling/FrameTimeline.h"
#include "Profiling/JsonWriter.h"
#include "Profiling/OutputFiles.h"
#include "Profiling/SystemInfo.h"

#include <algorithm>
#include <cstdio>

namespace GameLib{
namespace Profiling{

namespace{
	const double kMillisecondsPerSecond = 1000.0;
	const int kCellBufferSize = 64;
	// events 列は印の名前を ; でつなぐ（, は列の区切りなので使わない）。Excel の絞り込みで「Fire を含む」行だけを見られる。
	// gpuMs は GPU の時間が読めたフレームだけ（数フレーム遅れて届き、間に合わなければ捨てるので空のこともある）。
	const char* const kHeader = "frame,frameMs,cpuMs,presentMs,sleepMs,otherMs,gpuMs,ticks,state,wroteFile,events";
	const char kEventSeparator = ';';
	const float kNoGpu = -1.f; // GPU の時間がまだ・読めなかった
	// state 列の値。loading と suspended の行は、プレイ中の平均や p99 に混ぜないこと（計画書 6-6 ②）。
	const char* const kStatePlaying = "playing";
	const char* const kStateLoading = "loading";
	const char* const kStateSuspended = "suspended";

	const char* stateOf( const FrameRecord& frame ){
		if ( frame.suspended ){
			return kStateSuspended;
		}
		return frame.loading ? kStateLoading : kStatePlaying;
	}
}

FrameRecorder::FrameRecorder() :
mRecording( false ){
}

void FrameRecorder::start(){
	mRecording = true;
	mStartedAt = OutputFiles::timestamp();
	mRows.clear();
	mColumns.clear();
	mColumnOfName.clear();
}

void FrameRecorder::stop(){
	if ( !mRecording ){
		return;
	}
	mRecording = false;
	std::string baseName = "perf_" + mStartedAt;
	OutputFiles::write( OutputFiles::pathOf( baseName + ".csv" ), csvText() );
	JsonWriter json;
	json.beginObject( nullptr );
	json.integer( "frames", static_cast< long long >( mRows.size() ) );
	SystemInfo::write( "system", &json );
	json.endObject();
	OutputFiles::write( OutputFiles::pathOf( baseName + "_system.json" ), json.text() );
	mRows.clear();
}

bool FrameRecorder::isRecording() const {
	return mRecording;
}

int FrameRecorder::frameCount() const {
	return static_cast< int >( mRows.size() );
}

void FrameRecorder::add( const FrameRecord& frame ){
	if ( !mRecording ){
		return;
	}
	FrameTimes times = timesOf( frame );
	Row row;
	row.frame = frame.index;
	row.frameMs = static_cast< float >( times.frame * kMillisecondsPerSecond );
	row.cpuMs = static_cast< float >( times.cpu * kMillisecondsPerSecond );
	row.presentMs = static_cast< float >( times.present * kMillisecondsPerSecond );
	row.sleepMs = static_cast< float >( times.sleep * kMillisecondsPerSecond );
	row.otherMs = static_cast< float >( times.other * kMillisecondsPerSecond );
	row.gpuMs = kNoGpu; // 後で setGpu が入れる
	row.ticks = frame.ticks;
	row.state = stateOf( frame );
	row.wroteFile = frame.wroteFile;
	for ( int i = 0; i < frame.eventCount; ++i ){
		if ( i > 0 ){
			row.events += kEventSeparator;
		}
		row.events += frame.events[ i ];
	}
	// 同じ名前の区間（ティックごとの更新など）は 1 つの列に足す。
	for ( int i = 0; i < frame.scopeCount; ++i ){
		const ScopeEvent& scope = frame.scopes[ i ];
		int column = columnOf( scope.name );
		float ms = static_cast< float >( secondsOf( scope ) * kMillisecondsPerSecond );
		bool merged = false;
		for ( ScopeTotal& total : row.scopes ){
			if ( total.column == column ){
				total.milliseconds += ms;
				merged = true;
				break;
			}
		}
		if ( !merged ){
			row.scopes.push_back( { column, ms } );
		}
	}
	mRows.push_back( row );
}

void FrameRecorder::setGpu( long long frame, double seconds ){
	// 届くのは数フレーム前の分なので、後ろから探す。
	for ( auto it = mRows.rbegin(); it != mRows.rend(); ++it ){
		if ( it->frame == frame ){
			it->gpuMs = static_cast< float >( seconds * kMillisecondsPerSecond );
			return;
		}
		if ( it->frame < frame ){
			return; // 記録を始める前のフレーム
		}
	}
}

int FrameRecorder::columnOf( const char* name ){
	auto found = mColumnOfName.find( name );
	if ( found != mColumnOfName.end() ){
		return found->second;
	}
	int column = static_cast< int >( mColumns.size() );
	mColumns.push_back( name );
	mColumnOfName[ name ] = column;
	return column;
}

std::string FrameRecorder::csvText() const {
	std::string text = kHeader;
	for ( const std::string& column : mColumns ){
		text += ',';
		text += column;
	}
	text += '\n';
	char cell[ kCellBufferSize ];
	std::vector< float > scopeCells( mColumns.size() );
	for ( const Row& row : mRows ){
		snprintf( cell, sizeof( cell ), "%lld,%.3f,%.3f,%.3f,%.3f,%.3f,",
			row.frame, row.frameMs, row.cpuMs, row.presentMs, row.sleepMs, row.otherMs );
		text += cell;
		if ( row.gpuMs >= 0.f ){
			snprintf( cell, sizeof( cell ), "%.3f", row.gpuMs );
			text += cell;
		}
		snprintf( cell, sizeof( cell ), ",%d,%s,%d", row.ticks, row.state, row.wroteFile ? 1 : 0 );
		text += cell;
		text += ',';
		text += row.events;
		// その行に無い区間は 0（そのフレームでは走らなかった）。
		std::fill( scopeCells.begin(), scopeCells.end(), 0.f );
		for ( const ScopeTotal& total : row.scopes ){
			scopeCells[ static_cast< size_t >( total.column ) ] = total.milliseconds;
		}
		for ( float ms : scopeCells ){
			snprintf( cell, sizeof( cell ), ",%.3f", ms );
			text += cell;
		}
		text += '\n';
	}
	return text;
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
