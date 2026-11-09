#ifndef INCLUDED_GAMELIB_PROFILING_FRAME_RECORDER_H
#define INCLUDED_GAMELIB_PROFILING_FRAME_RECORDER_H

#ifdef ZF_EDITOR

#include "Profiling/GpuTimer.h"

#include <map>
#include <string>
#include <vector>

namespace GameLib{
namespace Profiling{

struct FrameRecord;

// F7 で始めてから止めるまでのフレームを、1 フレーム 1 行の CSV に書き出す。
// 記録中はメモリに貯めるだけで、ファイルへは止めたときにまとめて書く（記録中のフレームを重くしないため）。
// 区間（PROFILE_SCOPE）は名前ごとに列を作り、そのフレームの合計を入れる。列は止めた時点で全部分かっている。
// 同じ名前で PC の情報を JSON にして並べて保存する（CSV の先頭に書くと Excel で開きにくくなるため）。
//   例：Profiling/20261024_150000/perf_20261024_153000.csv と perf_20261024_153000_system.json
// CSV の 1 行目だけは記録した日時（startedAt,2026/10/24 15:30:00,stoppedAt,…）。列名は 2 行目から。
class FrameRecorder{
public:
	FrameRecorder();
	void start();
	// 止めて書き出す。
	void stop();
	bool isRecording() const;
	int frameCount() const;
	// 閉じたフレームを 1 行足す。
	void add( const FrameRecord& frame );
	// 数フレーム遅れて届いた GPU の時間を、その行に入れる（無い行なら何もしない）。
	// 全体（gpuMs 列）と、印（gpuMark）で分けた区間ごとの時間（"gpu.<区間の名前>" の列）の両方を入れる。
	void setGpu( const GpuTimer::Result& result );
private:
	struct ScopeTotal{
		int column;
		float milliseconds; // 回数（count. の列）のときは、その値のまま
	};
	struct Row{
		long long frame;
		float frameMs;
		float cpuMs;
		float presentMs;
		float sleepMs;
		float otherMs;
		float gpuMs;  // 負ならまだ・読めなかった
		int ticks;
		const char* state;
		bool wroteFile;
		std::string events; // このフレームの印を ; でつないだもの（例：Fire;Shot）
		std::vector< ScopeTotal > scopes;
	};
	int columnOf( const char* name );
	std::string csvText( const std::string& stoppedDateTime ) const;

	bool mRecording;
	std::string mStartedAt; // ファイル名に使う、始めた時刻
	std::string mStartedDateTime; // CSV の 1 行目に書く、始めた日時
	std::vector< Row > mRows;
	std::vector< std::string > mColumns;          // 区間の列の名前（見つけた順）
	std::map< std::string, int > mColumnOfName;
};

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
