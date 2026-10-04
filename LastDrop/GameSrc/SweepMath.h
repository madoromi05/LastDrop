#ifndef INCLUDED_SWEEP_MATH_H
#define INCLUDED_SWEEP_MATH_H

#include "GameLib/Math/Vector3.h"
#include "BoxCollider.h"

// 掃引で当たったときの結果。
struct SweepHit{
	float time;                    // 当たった時刻（0〜1。移動量のうち何割進んだところで当たったか）
	GameLib::Math::Vector3 normal; // 当たった面の法線（水平・長さ1。相手から動かした側へ向く）
};

// 掃引判定（動かした長方形が最初にどこで当たるか）と、めり込みを外へ出す量の計算だけを持つ。状態は持たない。
// どれも真上から見た 2D（XZ 平面）で解き、入力の y は無視する（返すベクトルの y は 0）。
//
// 回転した長方形どうしは、分離軸判定（SAT）で解く。調べる軸は、2つの長方形の辺の向き（それぞれ2本、計4本）。
// ・重なり：4本どの軸に影を落としても重なっていれば重なっている。一番浅い軸の向きへ押し出すのが最短。
// ・掃引：軸ごとに「影が重なり始める時刻・離れる時刻」を求める。全部の軸で重なっている一番早い時刻が当たった時刻で、
//   最後に重なり始めた軸がぶつかった面の向きになる。
class SweepMath{
public:
	// box を delta だけ動かしたとき、other に当たるか。当たれば hit に書いて true。
	// 動き始めから重なっていて、さらに奥へ進もうとしているときは time=0 で当たりとする（離れる向きなら当たらない）。
	// hit: 出力先。nullptr 不可。
	static bool sweepBox( const BoxCollider& box, const GameLib::Math::Vector3& delta, const BoxCollider& other, SweepHit* hit );

	// box が other にめり込んでいれば、外へ出す最短の移動量を push に書いて true。ちょうど接しているだけなら false。
	// push: 出力先。nullptr 不可。
	static bool penetrateBox( const BoxCollider& box, const BoxCollider& other, GameLib::Math::Vector3* push );
};

#endif
