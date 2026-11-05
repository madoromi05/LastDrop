#ifndef INCLUDED_FIRE_PROBE_H
#define INCLUDED_FIRE_PROBE_H

// 「ロード後、撃つボタンを 1 回押しても撃てない」の原因を調べる、仮の記録（FuelEscapeLoading_Plan.txt 4 章・F-0）。
// 撃つボタンを押した・離した描画ごとに 1 行、Profiling/fire_log.txt へ書く。押した後のティックで撃てたか（空撃ちか）も書く。
// 原因が決まって直したら、このクラスごと消す（ゲームの判断には使わない。読むだけ）。Debug・Profile（ZF_EDITOR）だけで使う。
class FireProbe{
public:
	// 描画ごとの入力と画面の状態（GameScene が beginFrame の最後に集めて渡す）。
	struct FrameState{
		bool rawFireHeld;   // 撃つボタンを押しているか（Pad そのもの）
		bool commandFire;   // 自キャラへ渡す「撃つ」（抑止・ロックを通した後）
		bool suppressed;    // 押しっぱなしの抑止（FirstPersonControl）
		bool cursorLocked;  // カーソルのロックを頼んでいるか
		bool windowActive;
		bool paused;
		bool bagOpen;
		bool lootOpen;
		bool freeCamera;
		bool dead;
	};
	// 持っている銃の状態（銃が無ければ hasGun = false で、ほかは読まない）。
	struct GunState{
		bool hasGun;
		bool inHand;     // しまっていない
		bool aiming;
		bool reloading;
		bool running;    // 走る入力
		int rounds;      // 弾倉の数
	};

	FireProbe();

	// ロードが終わった描画（数え始め）。
	void markReady();
	// 描画ごと（ロード後）。押した・離した描画だけ書く。
	void onFrame( const FrameState& frame, const GunState& gun );
	// ティックごと（世界を進めた後）。押してから決まったティックの間に撃てたかを書く。
	void onTick( bool fired, bool dryFired );

private:
	void write( const char* line ) const;

	int mFrame;          // ロードが終わってからの描画の数
	int mTicksThisFrame; // この描画で進めたティックの数（次の描画の行に書く）
	int mPressNumber;    // 何回目に押したか
	int mWatchTicks;     // 押してから見ている残りのティック（0 で見ていない）
	int mWatchedTicks;   // 押してから進んだティック
	bool mWasHeld;
	bool mReady;
};

#endif
