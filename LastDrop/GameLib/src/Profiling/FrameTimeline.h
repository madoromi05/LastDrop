#ifndef INCLUDED_GAMELIB_PROFILING_FRAME_TIMELINE_H
#define INCLUDED_GAMELIB_PROFILING_FRAME_TIMELINE_H

#ifdef ZF_EDITOR

#include <vector>

namespace GameLib{
namespace Profiling{

// 目標のフレームレート（2026-10-23 決定：60fps は必ず守る、144fps を目指す）。
const double kRequiredFps = 60.0;
const double kGoalFps = 144.0;
// 1 フレームの予算（秒）。スパイクの判定と画面の表示に使う。
const double kFrameBudgetSeconds = 1.0 / kRequiredFps;

// 1 区間の記録。
struct ScopeEvent{
	const char* name;
	int depth;       // 入れ子の深さ（0 が一番外）
	long long begin;
	long long end;   // 閉じる前は 0
};

// 1 フレームの記録。時刻は QueryPerformanceCounter の値。
struct FrameRecord{
	// 1 フレームに残す区間の上限。ティックを上限の 5 回回しても足りる数にしてある。
	static const int kMaxScopes = 256;
	// 1 フレームに残す印の上限。ボタンを同時にいくつか押し、弾が出てゾンビに当たっても足りる数。
	static const int kMaxEvents = 16;
	long long index;        // 起動してから何番目のフレームか
	long long begin;        // beginFrame
	long long sleepEnd;     // endLimiterSleep（呼ばれなければ begin と同じ）
	long long presentBegin;
	long long presentEnd;
	long long next;         // 次の beginFrame（閉じる前は 0）
	int ticks;              // この描画で進めたティック数
	bool loading;           // Present の時点でロード中だったか
	bool suspended;         // 最小化などで止まっていた（フレームの時間が意味を持たない）
	bool wroteFile;         // このフレームの中で計測のファイルを書いた（その分だけ遅い）
	int scopeCount;
	int droppedScopes;      // 上限を超えて残せなかった区間の数
	ScopeEvent scopes[ kMaxScopes ];
	int eventCount;
	int droppedEvents;      // 上限を超えて残せなかった印の数
	const char* events[ kMaxEvents ]; // このフレームに付いた印の名前（付いた順）
};

// 閉じたフレームの時間の内訳（秒）。frame = sleep + cpu + present + other。
struct FrameTimes{
	double frame;
	double sleep;   // フレーム制限の待ち
	double cpu;     // ゲームの処理と描画命令（待ちの後から Present の前まで）
	double present; // Present で待たされた時間
	double other;   // Present の後から次のフレームまで（窓のメッセージ処理）
};
FrameTimes timesOf( const FrameRecord& frame );
// 区間の長さ（秒）。閉じていなければ 0。
double secondsOf( const ScopeEvent& scope );

// 直近のフレームの記録を、決まった数だけ輪にして持つ（計画書 6-3）。
// 画面の表示・CSV・スパイクの書き出しは、ここから読む（計測のコードは 1 つ）。
class FrameTimeline{
public:
	static const int kHistory = 240; // 残すフレーム数（144Hz でも 1.6 秒ぶん）

	FrameTimeline();
	// 前のフレームを閉じて、新しいフレームを始める。閉じたフレームを返す（最初の 1 回は nullptr）。
	const FrameRecord* begin( long long at );
	// 今のフレーム。まだ 1 回も始まっていなければ nullptr。
	FrameRecord* current();
	// 区間を開く。残せなければ -1。
	int pushScope( const char* name, long long at );
	// pushScope で開いた区間を閉じる。開いた後にフレームが変わっていたら何もしない。
	void popScope( int slot, long long frameIndex, long long at );
	// 終わってから分かった区間（begin〜end）を、今の深さで足す。
	void addFinishedScope( const char* name, long long begin, long long end );
	// 今のフレームに印を足す。フレームが始まっていなければ何もしない。
	void addEvent( const char* name );
	// 閉じたフレームを新しい方から読む（ago = 0 が直前に閉じたフレーム）。無ければ nullptr。
	const FrameRecord* closed( int ago ) const;
private:
	FrameRecord& slotOf( long long index );
	const FrameRecord& slotOf( long long index ) const;

	std::vector< FrameRecord > mRing;
	long long mCurrentIndex; // 今のフレームの番号（始まっていなければ -1）
	int mDepth;              // 今開いている区間の数
};

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
