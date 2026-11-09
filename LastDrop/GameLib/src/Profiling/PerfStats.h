#ifndef INCLUDED_GAMELIB_PROFILING_PERF_STATS_H
#define INCLUDED_GAMELIB_PROFILING_PERF_STATS_H

#ifdef ZF_EDITOR

#include "Profiling/GpuTimer.h"

#include <vector>

namespace GameLib{
namespace Profiling{

class FrameTimeline;
class FrameRecorder;
class LoadTracker;
class EventLog;

// 画面の数字は 1 フレームごとだとばらついて読めないので、この数だけならす（0.5 秒ほど）。右上の表示・Perf 欄・律速の判定で同じ数を使う。
const int kAveragedFrames = 30;

// 直近のフレームの平均（秒）。止まっていたフレームは入れない。
struct FrameAverages{
	int frames;     // 平均に入れたフレームの数（0 なら他は意味が無い）
	double frame;
	double cpu;
	double present;
	double sleep;
	double other;
	double longest; // 一番長かったフレーム
};
FrameAverages averageFrames( const FrameTimeline& timeline, int frames );

// 読めた GPU の結果を、直近のものだけ輪にして持つ（GpuTimer の結果は数フレーム遅れて届く）。
class GpuHistory{
public:
	static const int kHistory = 240;
	GpuHistory();
	void add( const GpuTimer::Result& result );
	// 新しい方から（ago = 0 が一番新しい）。無ければ nullptr。
	const GpuTimer::Result* latest( int ago ) const;
	// 直近 frames 個の、フレーム全体の GPU の時間の平均（秒）。1 つも無ければ負。
	double averageTotal( int frames ) const;
private:
	std::vector< GpuTimer::Result > mRing;
	int mCount; // これまでに足した数
};

// CPU と GPU のどちらが重いか。
enum Bound{
	BOUND_UNKNOWN,  // GPU の時間がまだ読めていない
	BOUND_VSYNC,    // VSync 中は、CPU も GPU も画面の更新待ちに見えるので決めない
	BOUND_CPU,
	BOUND_GPU,
	BOUND_BALANCED,
};
const char* boundName( Bound bound );

// 平均の cpu と gpu の比で決める。境目でちらつかないよう、決め直すときと保つときで幅を変える（ヒステリシス）。
class BoundJudge{
public:
	BoundJudge();
	Bound update( bool vsync, double cpuSeconds, double gpuSeconds );
	Bound current() const;
private:
	Bound mBound;
};

// 右上の表示と Perf の欄が読む物をまとめて渡す（どちらも読むだけ）。
struct PerfSources{
	const FrameTimeline* timeline;
	const FrameRecorder* recorder;
	const LoadTracker* loads;
	const EventLog* events;
	const GpuHistory* gpu;
	Bound bound;
	int spikeFiles; // この起動で書いたスパイクの JSON の数
};

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
