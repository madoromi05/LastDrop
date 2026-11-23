#ifndef INCLUDED_ZOMBIE_MOVE_TRACE_H
#define INCLUDED_ZOMBIE_MOVE_TRACE_H

#include "GameLib/Math/Vector3.h"

// ゾンビ1体が引っかかったかを、画面に出すための動きの記録
// ZombieLocomotion が書き、ZombiePathDrawer・GameDebugOverlay（F6 の Zombie info の赤い印と番号・紫の線）が読む
// ゲームの判断には使わない
// 印を出すのは、ZombieLocomotion が mark を呼んだとき（よけ始めた・探すのをあきらめた・うろつく先を選び直した）と、
// 歩きたいティックが 1 秒続くたびにその 1 秒を確かめて、前へ進めていない・その場で回り続けていたと分かったとき
class ZombieMoveTrace{
public:
	ZombieMoveTrace();

	void beginTick();	// 画面の印を出す残りを減らす。ティックの頭で 1 回
	void mark();		// 引っかかった。少しの間、画面に印を出す
	void setAimPoint( const GameLib::Math::Vector3& point );	// 今向いている点（通り道の先読みの点・よける点）を覚える
	// 考えた結果。walking: 歩きたいか。facing: 向き終わっていて進めるか
	// 歩きたいティックを数え、1 秒たったらその 1 秒を確かめる。歩きたくなければ数え直す
	void noteIntent( bool walking, bool facing );
	// 歩いた 1 ティック。wanted: 進みたかった長さ。moved: 実際に動いた量。angleY: 歩いた向き（度）
	void noteStep( float wanted, const GameLib::Math::Vector3& moved, float angleY );

	bool isMarked() const;							// 最後に引っかかってから少しの間 true
	const GameLib::Math::Vector3& aimPoint() const;

private:
	void checkWindow();	// 数えた 1 秒を確かめ、引っかかっていれば印を出して数え直す
	void resetWindow();

	int mMarkTicks;	// 画面の印を出す残り（ティック）
	GameLib::Math::Vector3 mAimPoint;
	// ── 数えている 1 秒 ──
	int mWalkingTicks;
	int mTurningTicks;
	float mWanted;
	float mForward;	// 体の前へ進めた長さの和（m。壁に沿って横へ滑った分は数えない）
};

#endif
