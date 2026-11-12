#ifndef INCLUDED_GAMELIB_PROFILING_LOAD_TRACKER_H
#define INCLUDED_GAMELIB_PROFILING_LOAD_TRACKER_H

#ifdef ZF_EDITOR

#include <deque>
#include <map>
#include <string>
#include <vector>

namespace GameLib{
namespace Profiling{

struct FrameRecord;

// ロードの中の 1 件（ファイル 1 つの待ち・組み立て、シェーダ 1 つのコンパイルなど）
struct LoadItem{
	std::string category; // "wait"（別スレッドの読み込みを待った時間）以外は、メインスレッドの処理時間
	std::string name;
	double seconds;
	long long frame;      // どのフレームで起きたか（スパイクの内訳と突き合わせるため）
};

// ロードの時間を測る
// 始めてから「操作できる最初の絵」の Present までを 1 回のロードとし、終わったら Profiling/<起動時刻>/load_*.json を書き出す
// ロード中でなくても、読み込みの件は直近のものを残しておく（プレイ中のひっかかりの原因を、スパイクの JSON に出すため）
class LoadTracker{
public:
	// 直前に終わったロード（画面に出す）。まだ無ければ name が空
	struct Summary{
		static const int kSlowestCount = 5; // Perf の欄に出す、時間のかかった処理の数
		std::string name;
		int attempt;    // 同じ名前のロードの何回目か（Game の 2 回目は Retry。読み込み済みで速い）
		double seconds;
		std::vector< LoadItem > slowest; // メインスレッドの処理で時間のかかった順（待ち時間は入れない）
	};

	LoadTracker();
	void begin( const char* name, long long at );	// at からロードを始める。ロード中なら何もしない。name は文字列リテラル
	// 次の Present の後でロードを閉じる
	void requestEnd();
	bool isLoading() const;
	void addItem( const char* category, const char* name, double seconds, long long frame );
	// フレームの Present が終わった所で呼ぶ。終わりを頼まれていれば閉じて JSON を書く。書いたら true
	bool onPresented( const FrameRecord& frame );
	const std::deque< LoadItem >& recentItems() const;
	const Summary& lastSummary() const;
private:
	void finish( long long at );
	static int countLaunch();	// 何回目の起動か（Profiling/launch_count.txt に数えておく）。2 回目以降は OS がファイルを覚えていて速い

	bool mActive;
	bool mEndRequested;
	const char* mName;
	long long mBegin;
	int mFrames;
	double mLongestFrameSeconds; // ロード中で一番長かったフレーム（始めから Present まで）
	std::vector< LoadItem > mItems;
	std::deque< LoadItem > mRecent;
	std::map< std::string, int > mAttempts; // 名前ごとに何回ロードしたか
	int mLaunch;
	Summary mLast;
};

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
