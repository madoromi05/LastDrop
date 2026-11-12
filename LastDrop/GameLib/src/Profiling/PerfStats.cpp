#ifdef ZF_EDITOR

#include "Profiling/PerfStats.h"
#include "Profiling/FrameTimeline.h"

#include <algorithm>

namespace GameLib{
namespace Profiling{

namespace{
	const double kEnterRatio = 1.25;	// 片方がもう片方のこの倍を超えたら、そちらが重いと決め直す
	const double kStayRatio = 1.1;		// この倍の内側に戻ったら、釣り合っているとする（間の幅では前の判定を保つ）
}

FrameAverages averageFrames( const FrameTimeline& timeline, int frames ){
	FrameAverages a = {};
	for ( int ago = 0; ago < frames; ++ago ){
		const FrameRecord* frame = timeline.closed( ago );
		if ( !frame ){
			break;
		}
		if ( frame->suspended ){
			continue;
		}
		FrameTimes t = timesOf( *frame );
		a.frame += t.frame;
		a.cpu += t.cpu;
		a.present += t.present;
		a.sleep += t.sleep;
		a.other += t.other;
		a.longest = std::max( a.longest, t.frame );
		++a.frames;
	}
	if ( a.frames > 0 ){
		a.frame /= a.frames;
		a.cpu /= a.frames;
		a.present /= a.frames;
		a.sleep /= a.frames;
		a.other /= a.frames;
	}
	return a;
}

GpuHistory::GpuHistory() :
mRing( kHistory ),
mCount( 0 ){
}

void GpuHistory::add( const GpuTimer::Result& result ){
	mRing[ mCount % kHistory ] = result;
	++mCount;
}

const GpuTimer::Result* GpuHistory::latest( int ago ) const {
	if ( ago < 0 || ago >= kHistory || ago >= mCount ){
		return nullptr;
	}
	return &mRing[ ( mCount - 1 - ago ) % kHistory ];
}

double GpuHistory::averageTotal( int frames ) const {
	double sum = 0.0;
	int count = 0;
	for ( int ago = 0; ago < frames; ++ago ){
		const GpuTimer::Result* result = latest( ago );
		if ( !result ){
			break;
		}
		sum += result->totalSeconds;
		++count;
	}
	return ( count > 0 ) ? sum / count : -1.0;
}

const char* boundName( Bound bound ){
	switch ( bound ){
	case BOUND_VSYNC:    return "VSync (turn off to judge)";
	case BOUND_CPU:      return "CPU";
	case BOUND_GPU:      return "GPU";
	case BOUND_BALANCED: return "Balanced";
	default:             return "--";
	}
}

BoundJudge::BoundJudge() :
mBound( BOUND_UNKNOWN ){
}

Bound BoundJudge::update( bool vsync, double cpuSeconds, double gpuSeconds ){
	if ( vsync ){
		mBound = BOUND_VSYNC;
		return mBound;
	}
	if ( gpuSeconds < 0.0 || cpuSeconds <= 0.0 ){
		mBound = BOUND_UNKNOWN;
		return mBound;
	}
	double ratio = gpuSeconds / cpuSeconds;
	if ( ratio >= kEnterRatio ){
		mBound = BOUND_GPU;
	}else if ( ratio <= 1.0 / kEnterRatio ){
		mBound = BOUND_CPU;
	}else if ( ratio < kStayRatio && ratio > 1.0 / kStayRatio ){
		mBound = BOUND_BALANCED;
	}else if ( mBound != BOUND_CPU && mBound != BOUND_GPU ){
		mBound = BOUND_BALANCED; // 幅の中で、保つ判定がまだ無い
	}
	return mBound;
}

Bound BoundJudge::current() const {
	return mBound;
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
