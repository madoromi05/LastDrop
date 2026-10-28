#ifndef INCLUDED_GAMELIB_PROFILING_GPU_TIMER_H
#define INCLUDED_GAMELIB_PROFILING_GPU_TIMER_H

#ifdef ZF_EDITOR

struct ID3D11Query;

namespace GameLib{
namespace Profiling{

// GPU の時間を D3D11 のタイムスタンプで測る（計画書 7-1）。
// フレームの始めと終わり、その間の印（gpuMark）で時刻を打ち、印と印の間をその区間の GPU の時間とする。
// 区間の名前は、終わりの印の名前（"Scene" ならシーンを描き終えるまで）。最後の印から Present までは "Rest"。
//
// 結果は kLatency フレーム後に読む（同じフレームで待つと CPU が GPU を待って止まり、計測が歪むため）。
// 読む時に間に合っていなければ、そのフレームは捨てる。クロックが変わったフレーム（Disjoint）も捨てる。
class GpuTimer{
public:
	static const int kLatency = 5;   // 何フレーム分のクエリを輪にして回すか
	// 1 フレームに打てる印の数（始めと終わりは別）。超えた印は黙って捨てる。
	// 2026-10-27 に世界を描く所を物ごとに分けて 9 個になったので、足す余地を残して 12。
	static const int kMaxMarks = 12;

	// 読めた 1 フレームぶんの結果。
	struct Result{
		long long frame;
		double totalSeconds;                 // 始め〜終わり
		int segmentCount;
		const char* names[ kMaxMarks + 1 ];  // 区間の名前（最後は "Rest"）
		double segmentSeconds[ kMaxMarks + 1 ];
	};

	GpuTimer();
	~GpuTimer();
	// クエリを作る。Graphics::Manager ができた後に、最初の beginFrame で呼ばれる。
	// フレームの始め（Profiling::beginFrame）。
	void beginFrame( long long frameIndex );
	void mark( const char* name );
	// フレームの終わり（Present の直前）。一番古いフレームの結果が読めたら result に入れて true。
	bool endFrame( Result* result );
	// クエリを捨てる（Graphics::Manager を消す前）。
	void release();
private:
	struct Slot{
		ID3D11Query* disjoint;
		ID3D11Query* stamps[ kMaxMarks + 2 ]; // 始め + 印 + 終わり
		const char* names[ kMaxMarks + 1 ];
		int markCount;
		long long frame;
		bool issued;  // End まで打ったか（読む対象か）
	};
	bool create();
	bool read( Slot& slot, Result* result );

	Slot mSlots[ kLatency ];
	bool mCreated;
	bool mFailed;   // 作れなかった（D3D のデバイスが無い等）。以後は何もしない
	bool mInFrame;  // beginFrame から endFrame の間か
	int mCurrent;   // 今のフレームの Slot
};

} //namespace Profiling
} //namespace GameLib

#endif // ZF_EDITOR

#endif
