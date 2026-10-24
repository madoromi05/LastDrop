#ifndef INCLUDED_GAMELIB_PROFILING_PROFILER_H
#define INCLUDED_GAMELIB_PROFILING_PROFILER_H

// 性能の計測（フレーム時間・区間・ロード）の窓口。計画書は SummaryPlan/個別仕様/DebugTools_Profiling_Plan.txt の 6 章。
// Debug と Profile ビルド（ZF_EDITOR）だけで動く。Release ではすべて何もしない関数とマクロになるので、
// 呼ぶ側に #ifdef を書かなくてよい。
// メインスレッドからだけ使う（ほかのスレッドから呼んだ区間は記録しない。区間の入れ子が崩れるため）。
//
// 1 フレームの時間は次のように分けて測る（計画書 2 章）。
//   frame = sleep（フレーム制限の待ち）+ cpu（ゲームの処理と描画命令）+ present（Present で待たされた時間）+ other（窓のメッセージ処理）
// 画面の右上に数字を出し、F7 で CSV に記録する。書き出し先は Profiling/（.gitignore 済み）。
namespace GameLib{
namespace Profiling{

#ifdef ZF_EDITOR

// 高精度の時刻（QueryPerformanceCounter の値）。
long long now();
// 2つの時刻の差（秒）。
double secondsBetween( long long from, long long to );

// ── フレームの区切り（GameLib の Framework と Graphics::Manager、SceneManager だけが呼ぶ）──
void beginFrame();      // Framework::preUpdate の先頭。前のフレームを閉じ、新しいフレームを始める
void endLimiterSleep(); // フレーム制限の待ち（sleep）が終わった所
void beginPresent();    // Present の直前
void endPresent();      // Present の直後
void setTicks( int ticks ); // この描画でゲームを何ティック進めたか
void markSuspended();   // 最小化などでフレームを回さずに待った。今のフレームは時間が意味を持たないので平均・スパイクから外す
void shutdown();        // 終了時（Graphics::Manager を消す前）。記録中の CSV を書き出す

// ── 区間 ──
// 作ってから消えるまでの時間を、今のフレームの区間として残す。PROFILE_SCOPE マクロで使う。
// name は文字列リテラル（ポインタを持ち続ける）。
class Scope{
public:
	explicit Scope( const char* name );
	~Scope();
private:
	Scope( const Scope& );
	void operator=( const Scope& );
	int mSlot;             // フレームの中の何番目の区間か（記録しないときは -1）
	long long mFrameIndex; // 作ったときのフレーム（フレームをまたいだら記録しない）
};

// ── ロード（計画書 6-6）──
// 起動からの "Boot"。プロセスが作られた時刻から数える（窓やデバイスを作る時間も入る）。Framework::start の先頭で呼ぶ。
void beginBootLoad();
// シーンの切り替えなどでロードを始める。既にロード中なら何もしない（続けて 1 つのロードとして数える）。
// name は文字列リテラル。
void beginLoad( const char* name );
// 操作できる最初の絵を描いた。次の Present の後でロードを閉じ、Profiling/load_*.json を書き出す。ロード中でなければ何もしない。
void endLoad();
bool isLoading();
// 別スレッドの読み込みを待った時間（requestedAt に頼んで、今届いたと分かった）。ファイルごとに 1 回呼ぶ。
void addLoadWait( const char* name, long long requestedAt );
// メインスレッドで beganAt から今までかかった処理（読み終えた文書の解析など、終わってから分かるもの）。
// category は文字列リテラル。区間（PROFILE_SCOPE）としても残す。
void addLoadWork( const char* category, const char* name, long long beganAt );
// メインスレッドの処理を、作ってから消えるまで測る addLoadWork。LOAD_SCOPE マクロで使う。
// category は文字列リテラル、name は消えるまで生きていればよい（写して持つ）。
class LoadScope{
public:
	LoadScope( const char* category, const char* name );
	~LoadScope();
private:
	LoadScope( const LoadScope& );
	void operator=( const LoadScope& );
	const char* mCategory;
	const char* mName;
	long long mBegin;
};

// ── 印（イベント）──
// 今のフレームに「何をしたか」の印を付ける（押したボタン・弾が出た・ゾンビが湧いた など）。PROFILE_EVENT マクロで使う。
// 印の付いたフレームの時間と、その後のフレームの一番長い時間を、CSV・スパイクの JSON・右上の表示で見られる。
// name は文字列リテラル（ポインタを持ち続ける）。1 フレームに残すのは FrameRecord::kMaxEvents 個まで。
void markEvent( const char* name );

// ── GPU の時間（計画書 7 章）──
// GPU に時刻を打つ。前の印からここまでが name の区間の GPU の時間になる（例："Scene" はシーンを描き終えるまで）。
// フレームの始めと Present の前は自動で打つ。name は文字列リテラル。1 フレーム GpuTimer::kMaxMarks 個まで。
void gpuMark( const char* name );

// ── 記録・表示 ──
void toggleRecording(); // CSV の記録を始める／止めて書き出す（F7）
void drawOverlay();     // 画面の右上に数字を出す（ImGui。エディタを開いている間は出さない）。Framework::postUpdate で呼ぶ
void drawPerfPanel();   // エディタの Perf の欄の中身（グラフ・区間・GPU・印・ロード）。EditorWindow が ImGui の窓の中で呼ぶ

#else // ZF_EDITOR

inline long long now(){ return 0; }
inline double secondsBetween( long long, long long ){ return 0.0; }
inline void beginFrame(){}
inline void endLimiterSleep(){}
inline void beginPresent(){}
inline void endPresent(){}
inline void setTicks( int ){}
inline void markSuspended(){}
inline void shutdown(){}
inline void beginBootLoad(){}
inline void beginLoad( const char* ){}
inline void endLoad(){}
inline bool isLoading(){ return false; }
inline void addLoadWait( const char*, long long ){}
inline void addLoadWork( const char*, const char*, long long ){}
inline void markEvent( const char* ){}
inline void gpuMark( const char* ){}
inline void toggleRecording(){}
inline void drawOverlay(){}
inline void drawPerfPanel(){}

#endif // ZF_EDITOR

} //namespace Profiling
} //namespace GameLib

#ifdef ZF_EDITOR
#define GAMELIB_PROFILING_CONCAT_INNER( a, b ) a##b
#define GAMELIB_PROFILING_CONCAT( a, b ) GAMELIB_PROFILING_CONCAT_INNER( a, b )
// この行から括弧の終わりまでを、name の区間として測る。
#define PROFILE_SCOPE( name ) GameLib::Profiling::Scope GAMELIB_PROFILING_CONCAT( profilingScope, __LINE__ )( name )
// この行から括弧の終わりまでを、ロードの処理（category の name）として測る。
#define LOAD_SCOPE( category, name ) GameLib::Profiling::LoadScope GAMELIB_PROFILING_CONCAT( profilingLoadScope, __LINE__ )( category, name )
// 今のフレームに name の印を付ける。
#define PROFILE_EVENT( name ) GameLib::Profiling::markEvent( name )
#else
#define PROFILE_SCOPE( name ) ( ( void )0 )
#define LOAD_SCOPE( category, name ) ( ( void )0 )
#define PROFILE_EVENT( name ) ( ( void )0 )
#endif

#endif
