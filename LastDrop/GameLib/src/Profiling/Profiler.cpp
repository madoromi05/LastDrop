#ifdef ZF_EDITOR

#include <windows.h>
#undef min
#undef max

#include "GameLib/Profiling/Profiler.h"
#include "GameLib/WindowCreator/WindowCreator.h"
#include "GameLib/Graphics/Manager.h"
#include "GameLib/DebugUi/DebugUi.h"
#include "Profiling/EventLog.h"
#include "Profiling/FrameTimeline.h"
#include "Profiling/FrameRecorder.h"
#include "Profiling/GpuTimer.h"
#include "Profiling/LoadTracker.h"
#include "Profiling/PerfOverlay.h"
#include "Profiling/PerfPanel.h"
#include "Profiling/PerfStats.h"
#include "Profiling/SpikeWriter.h"

namespace GameLib{
namespace Profiling{

namespace{
	const char* const kBootLoadName = "Boot";
	const char* const kWaitCategory = "wait";
	const long long kFileTimeUnitsPerSecond = 10000000; // FILETIME は 100 ナノ秒単位

	// 計測の部品をまとめて持つ。最初に使ったときに作り、終了まで持つ。
	struct State{
		FrameTimeline timeline;
		FrameRecorder recorder;
		LoadTracker loads;
		SpikeWriter spikes;
		EventLog events;
		GpuTimer gpu;
		GpuHistory gpuHistory;
		BoundJudge bound;
	};

	State& state(){
		static State s;
		return s;
	}

	PerfSources sourcesOf( const State& s ){
		PerfSources sources;
		sources.timeline = &s.timeline;
		sources.recorder = &s.recorder;
		sources.loads = &s.loads;
		sources.events = &s.events;
		sources.gpu = &s.gpuHistory;
		sources.bound = s.bound.current();
		sources.spikeFiles = s.spikes.writtenCount();
		return sources;
	}

	long long frequency(){
		static long long f = 0;
		if ( f == 0 ){
			LARGE_INTEGER value;
			QueryPerformanceFrequency( &value );
			f = value.QuadPart;
		}
		return f;
	}

	long long currentFrameIndex(){
		FrameRecord* frame = state().timeline.current();
		return frame ? frame->index : -1;
	}

	// プロセスが作られた時刻を、QueryPerformanceCounter の時刻に直す。
	long long processStartTime(){
		long long nowCounter = now();
		FILETIME creation, exitTime, kernel, user;
		if ( !GetProcessTimes( GetCurrentProcess(), &creation, &exitTime, &kernel, &user ) ){
			return nowCounter;
		}
		FILETIME current;
		GetSystemTimeAsFileTime( &current );
		ULARGE_INTEGER created, nowFileTime;
		created.LowPart = creation.dwLowDateTime;
		created.HighPart = creation.dwHighDateTime;
		nowFileTime.LowPart = current.dwLowDateTime;
		nowFileTime.HighPart = current.dwHighDateTime;
		long long elapsedUnits = static_cast< long long >( nowFileTime.QuadPart - created.QuadPart );
		return nowCounter - elapsedUnits * frequency() / kFileTimeUnitsPerSecond;
	}
}

long long now(){
	LARGE_INTEGER value;
	QueryPerformanceCounter( &value );
	return value.QuadPart;
}

double secondsBetween( long long from, long long to ){
	return static_cast< double >( to - from ) / static_cast< double >( frequency() );
}

void beginFrame(){
	State& s = state();
	const FrameRecord* closed = s.timeline.begin( now() );
	s.gpu.beginFrame( s.timeline.current()->index ); // GPU にもフレームの始めの時刻を打つ
	if ( !closed ){
		return;
	}
	s.recorder.add( *closed );
	s.events.onFrameClosed( *closed );
	// 書き出しは新しいフレームの中で起きるので、そのフレームに印を付ける（遅い理由が分かるように）。
	if ( s.spikes.writeIfSpike( *closed, s.loads.recentItems() ) ){
		s.timeline.current()->wroteFile = true;
	}
	// CPU と GPU のどちらが重いかは、閉じたフレームが増えるたびに決め直す（表示は右上と Perf の欄）。
	FrameAverages averages = averageFrames( s.timeline, kAveragedFrames );
	s.bound.update( Graphics::Manager().isVSync(), averages.cpu, s.gpuHistory.averageTotal( kAveragedFrames ) );
}

void endLimiterSleep(){
	FrameRecord* frame = state().timeline.current();
	if ( frame ){
		frame->sleepEnd = now();
	}
}

void beginPresent(){
	State& s = state();
	// GPU にフレームの終わりの時刻を打ち、数フレーム前の結果が読めたら残す。
	GpuTimer::Result result;
	if ( s.gpu.endFrame( &result ) ){
		s.gpuHistory.add( result );
		s.recorder.setGpu( result );
	}
	FrameRecord* frame = s.timeline.current();
	if ( frame ){
		frame->presentBegin = now();
	}
}

void endPresent(){
	State& s = state();
	FrameRecord* frame = s.timeline.current();
	if ( !frame ){
		return;
	}
	frame->presentEnd = now();
	frame->loading = s.loads.isLoading();
	if ( s.loads.onPresented( *frame ) ){
		frame->wroteFile = true;
	}
}

void setTicks( int ticks ){
	FrameRecord* frame = state().timeline.current();
	if ( frame ){
		frame->ticks = ticks;
	}
}

void markSuspended(){
	FrameRecord* frame = state().timeline.current();
	if ( frame ){
		frame->suspended = true;
	}
}

void shutdown(){
	State& s = state();
	s.recorder.stop();
	s.gpu.release(); // D3D11 のクエリなので、デバイスより先に捨てる
}

Scope::Scope( const char* name ) :
mSlot( -1 ),
mFrameIndex( -1 ){
	if ( !WindowCreator::isMainThread() ){
		return;
	}
	FrameTimeline& timeline = state().timeline;
	FrameRecord* frame = timeline.current();
	if ( !frame ){
		return;
	}
	mFrameIndex = frame->index;
	mSlot = timeline.pushScope( name, now() );
}

Scope::~Scope(){
	if ( mSlot >= 0 ){
		state().timeline.popScope( mSlot, mFrameIndex, now() );
	}
}

void beginBootLoad(){
	state().loads.begin( kBootLoadName, processStartTime() );
}

void beginLoad( const char* name ){
	state().loads.begin( name, now() );
}

void endLoad(){
	state().loads.requestEnd();
}

bool isLoading(){
	return state().loads.isLoading();
}

void addLoadWait( const char* name, long long requestedAt ){
	if ( !WindowCreator::isMainThread() ){
		return;
	}
	state().loads.addItem( kWaitCategory, name, secondsBetween( requestedAt, now() ), currentFrameIndex() );
}

void addLoadWork( const char* category, const char* name, long long beganAt ){
	if ( !WindowCreator::isMainThread() ){
		return;
	}
	long long end = now();
	State& s = state();
	s.timeline.addFinishedScope( category, beganAt, end );
	s.loads.addItem( category, name, secondsBetween( beganAt, end ), currentFrameIndex() );
}

LoadScope::LoadScope( const char* category, const char* name ) :
mCategory( category ),
mName( name ),
mBegin( now() ){
}

LoadScope::~LoadScope(){
	addLoadWork( mCategory, mName, mBegin );
}

void markEvent( const char* name ){
	if ( !WindowCreator::isMainThread() ){
		return;
	}
	state().timeline.addEvent( name );
}

void gpuMark( const char* name ){
	if ( !WindowCreator::isMainThread() ){
		return;
	}
	state().gpu.mark( name );
}

void toggleRecording(){
	State& s = state();
	if ( s.recorder.isRecording() ){
		s.recorder.stop();
		FrameRecord* frame = s.timeline.current();
		if ( frame ){
			frame->wroteFile = true;
		}
	}else{
		s.recorder.start();
	}
}

void drawOverlay(){
	// エディタを開いている間は、同じ数字が Perf の欄に出るので出さない。
	if ( DebugUi::isOpen() ){
		return;
	}
	PerfOverlay::draw( sourcesOf( state() ) );
}

void drawPerfPanel(){
	PerfPanel::draw( sourcesOf( state() ) );
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
