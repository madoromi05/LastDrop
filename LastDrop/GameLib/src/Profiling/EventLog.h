#ifndef INCLUDED_GAMELIB_PROFILING_EVENT_LOG_H
#define INCLUDED_GAMELIB_PROFILING_EVENT_LOG_H

#ifdef ZF_EDITOR

#include <deque>

namespace GameLib{
namespace Profiling{

struct FrameRecord;

// 印 1 つぶんの記録
struct EventRecord{
	const char* name;
	long long frame;          // 印の付いたフレーム
	double frameSeconds;      // そのフレームの時間
	double cpuSeconds;        // そのフレームの cpu の時間
	double worstAfterSeconds; // そのフレームから EventLog::kFollowFrames 枚の中で、一番長いフレームの時間
	int framesSeen;           // worstAfterSeconds に入れたフレームの数（kFollowFrames になったら決まり）
};

// 印（PROFILE_EVENT・押したボタン）の付いたフレームが、どれだけ重かったかをまとめる
// 弾を撃った後に音や血しぶきの準備が少し遅れて来ることもあるので、印のフレームだけでなく
// 後に続くフレームも見る。直近のものだけを残す（画面の表示・ImGui の Perf 欄で読む）
class EventLog{
public:
	static const int kFollowFrames = 10;     // 印のフレームを含めて、何フレーム見るか
	static const size_t kRecentLimit = 64;   // 残す印の数

	void onFrameClosed( const FrameRecord& frame );		// 閉じたフレームを渡す。前の印の「後のフレーム」を更新し、このフレームの印を足す
	const std::deque< EventRecord >& recent() const;	// 古い順
private:
	std::deque< EventRecord > mRecent;
};

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
