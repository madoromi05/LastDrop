#ifdef ZF_EDITOR

#include "Profiling/FrameTimeline.h"
#include "GameLib/Profiling/Profiler.h"

namespace GameLib{
namespace Profiling{

FrameTimes timesOf( const FrameRecord& frame ){
	FrameTimes t;
	t.frame = secondsBetween( frame.begin, frame.next );
	t.sleep = secondsBetween( frame.begin, frame.sleepEnd );
	t.cpu = secondsBetween( frame.sleepEnd, frame.presentBegin );
	t.present = secondsBetween( frame.presentBegin, frame.presentEnd );
	t.other = secondsBetween( frame.presentEnd, frame.next );
	return t;
}

double secondsOf( const ScopeEvent& scope ){
	if ( scope.end < scope.begin ){
		return 0.0;
	}
	return secondsBetween( scope.begin, scope.end );
}

FrameTimeline::FrameTimeline() :
mRing( kHistory ),
mCurrentIndex( -1 ),
mDepth( 0 ){
}

const FrameRecord* FrameTimeline::begin( long long at ){
	const FrameRecord* closedFrame = nullptr;
	if ( mCurrentIndex >= 0 ){
		FrameRecord& previous = slotOf( mCurrentIndex );
		previous.next = at;
		closedFrame = &previous;
	}
	++mCurrentIndex;
	FrameRecord& frame = slotOf( mCurrentIndex );
	frame.index = mCurrentIndex;
	frame.begin = at;
	// 呼ばれなかった区切りは、直前の区切りと同じ時刻にしておく（その区間の長さが 0 になる）。
	frame.sleepEnd = at;
	frame.presentBegin = at;
	frame.presentEnd = at;
	frame.next = 0;
	frame.ticks = 0;
	frame.loading = false;
	frame.suspended = false;
	frame.wroteFile = false;
	frame.scopeCount = 0;
	frame.droppedScopes = 0;
	frame.eventCount = 0;
	frame.droppedEvents = 0;
	mDepth = 0; // 前のフレームで閉じ忘れた区間は持ち越さない
	return closedFrame;
}

FrameRecord* FrameTimeline::current(){
	if ( mCurrentIndex < 0 ){
		return nullptr;
	}
	return &slotOf( mCurrentIndex );
}

int FrameTimeline::pushScope( const char* name, long long at ){
	FrameRecord* frame = current();
	if ( !frame ){
		return -1;
	}
	if ( frame->scopeCount >= FrameRecord::kMaxScopes ){
		++frame->droppedScopes;
		return -1;
	}
	int slot = frame->scopeCount;
	ScopeEvent& scope = frame->scopes[ slot ];
	scope.name = name;
	scope.depth = mDepth;
	scope.begin = at;
	scope.end = 0;
	++frame->scopeCount;
	++mDepth;
	return slot;
}

void FrameTimeline::popScope( int slot, long long frameIndex, long long at ){
	if ( slot < 0 || frameIndex != mCurrentIndex ){
		return;
	}
	slotOf( mCurrentIndex ).scopes[ slot ].end = at;
	if ( mDepth > 0 ){
		--mDepth;
	}
}

void FrameTimeline::addFinishedScope( const char* name, long long begin, long long end ){
	int slot = pushScope( name, begin );
	if ( slot >= 0 ){
		popScope( slot, mCurrentIndex, end );
	}
}

void FrameTimeline::addEvent( const char* name ){
	FrameRecord* frame = current();
	if ( !frame ){
		return;
	}
	if ( frame->eventCount >= FrameRecord::kMaxEvents ){
		++frame->droppedEvents;
		return;
	}
	frame->events[ frame->eventCount ] = name;
	++frame->eventCount;
}

const FrameRecord* FrameTimeline::closed( int ago ) const {
	// 今のフレームはまだ閉じていないので、その 1 つ前から数える。
	long long index = mCurrentIndex - 1 - ago;
	if ( ago < 0 || index < 0 || ago >= kHistory - 1 ){
		return nullptr;
	}
	return &slotOf( index );
}

FrameRecord& FrameTimeline::slotOf( long long index ){
	return mRing[ static_cast< size_t >( index % kHistory ) ];
}

const FrameRecord& FrameTimeline::slotOf( long long index ) const {
	return mRing[ static_cast< size_t >( index % kHistory ) ];
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
