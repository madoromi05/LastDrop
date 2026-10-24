#ifndef INCLUDED_GAMELIB_PROFILING_SPIKE_WRITER_H
#define INCLUDED_GAMELIB_PROFILING_SPIKE_WRITER_H

#ifdef ZF_EDITOR

#include <deque>

namespace GameLib{
namespace Profiling{

struct FrameRecord;
struct LoadItem;

// 予算を大きく超えたフレーム（スパイク）の内訳を、自動で Profiling/spike_*.json に書き出す（計画書 6-4・6-6 ③）。
// 区間の一覧と、そのフレームで起きた読み込み（初めて使ったテクスチャなど）を並べるので、
// ゲーム中に時々カクッとする原因を名前で追える。
// 書き出し自体も重いので、続けて書かない・1 回の起動で書く数にも上限を付ける。
class SpikeWriter{
public:
	SpikeWriter();
	// 閉じたフレームがスパイクなら書き出す。書いたら true。
	bool writeIfSpike( const FrameRecord& frame, const std::deque< LoadItem >& recentLoadItems );
	// この起動で書いた数（Perf の欄に出す）。
	int writtenCount() const;
private:
	long long mLastWrittenFrame; // 前に書いたフレーム（まだなら負）
	double mSecondsSinceWritten; // 前に書いてから閉じたフレームの時間の合計
	int mWrittenCount;
};

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
