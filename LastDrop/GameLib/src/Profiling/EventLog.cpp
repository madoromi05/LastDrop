#ifdef ZF_EDITOR

#include "Profiling/EventLog.h"
#include "Profiling/FrameTimeline.h"

#include <algorithm>

namespace GameLib{
namespace Profiling{

void EventLog::onFrameClosed( const FrameRecord& frame ){
	// 止まっていたフレームは時間に意味が無いので、印も後の更新も数えない。
	if ( frame.suspended ){
		return;
	}
	FrameTimes times = timesOf( frame );
	// 新しい方から見て、まだ決まっていない印の「後のフレーム」を更新する。
	for ( auto it = mRecent.rbegin(); it != mRecent.rend(); ++it ){
		if ( it->framesSeen >= kFollowFrames ){
			break; // これより古い印はもう決まっている
		}
		it->worstAfterSeconds = std::max( it->worstAfterSeconds, times.frame );
		++it->framesSeen;
	}
	for ( int i = 0; i < frame.eventCount; ++i ){
		EventRecord record;
		record.name = frame.events[ i ];
		record.frame = frame.index;
		record.frameSeconds = times.frame;
		record.cpuSeconds = times.cpu;
		record.worstAfterSeconds = times.frame; // 印のフレーム自身も「後のフレーム」の 1 枚目に数える
		record.framesSeen = 1;
		mRecent.push_back( record );
		if ( mRecent.size() > kRecentLimit ){
			mRecent.pop_front();
		}
	}
}

const std::deque< EventRecord >& EventLog::recent() const {
	return mRecent;
}

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR
