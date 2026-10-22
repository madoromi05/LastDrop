#ifndef INCLUDED_ZOMBIE_PATH_H
#define INCLUDED_ZOMBIE_PATH_H

#include "GameLib/Math/Vector3.h"
#include <vector>

class StreetGraph;
class HitWorld;

// ゾンビ1体の通り道（建物を回り込むときだけ使う）。行き先（どこへ行きたいか）は ZombieChase が決め、
// ここは「このティックにどこを向いて歩くか」だけを答える。自分では動かない（動くのは Zombie）。
// 決まった間隔で、行き先へまっすぐ通れるかを調べる（体ごとに時刻をずらす）。
//   通る  ：通り道を捨てて、行き先へまっすぐ向かう（今までと同じ動き）
//   当たる：通り道が無いか、行き先が前に探したときから大きく動いていたら StreetGraph に探し直させる
// 通り道の点に近づいたら次の点へ。次の次の点が見えていれば、今の点を飛ばす（角で交差点の真ん中まで寄らない）。
// 向く点へ近づけているかも数える（見失った後に着けない所で押し続けないため。あきらめるかは持ち主が決める）。
// 行き先が 1 ティックで大きく飛んだら（新しい銃声の所へ向かい直した）、すぐ調べ直して数え直す。
// 値の理由は SummaryPlan/個別仕様/ZombiePath_Plan.txt。
class ZombiePath{
public:
	// probePhase: まっすぐ通れるかを調べる時刻をずらす番号（何体目か）。同じティックに全員が線を引かないようにする。
	explicit ZombiePath( int probePhase );

	// このティックに向く点を返す。self: 自分の足元。destination: 最後に向かいたい所（ZombieChase::destination）。
	// 毎ティック呼ぶ（間隔を数え、近づけているかを数えるため）。
	const GameLib::Math::Vector3& steer( const GameLib::Math::Vector3& self, const GameLib::Math::Vector3& destination,
		const StreetGraph& streets, const HitWorld& hitWorld );
	// 通り道を捨てる（うろつきに戻った・攻撃を始めた）。次の steer ですぐ、まっすぐ通れるかを調べる。
	void clear();
	// 向く点へ、決まった時間近づけていないか（見失った後、着けない所へ向かっているのを見分ける）。
	bool isStuck() const;
	// 近づけているかを数え直す（探し始めたとき。追っている間に数えた分を持ち越さない）。
	void resetProgress();
	// 残りの通り道（今向かっている点から最後＝行き先まで）。まっすぐ向かっている間は空。確認用の表示に使う。
	// out: nullptr 不可。前の中身は捨てる。
	void remainingRoute( std::vector< GameLib::Math::Vector3 >* out ) const;

private:
	// 決まった間隔で、まっすぐ通れるか・探し直すか・点を飛ばせるかを決める。
	void probe( const GameLib::Math::Vector3& self, const GameLib::Math::Vector3& destination,
		const StreetGraph& streets, const HitWorld& hitWorld );
	// 今向かっている点（通り道が無ければ行き先）。
	const GameLib::Math::Vector3& currentPoint( const GameLib::Math::Vector3& destination ) const;

	std::vector< GameLib::Math::Vector3 > mRoute; // 通り道（最後は行き先）。空ならまっすぐ向かう
	int mNext;                                    // mRoute のうち今向かっている点の番号
	GameLib::Math::Vector3 mRouteGoal;            // 通り道を探したときの行き先（大きく動いたら探し直す）
	GameLib::Math::Vector3 mLastDestination;      // 前のティックの行き先（飛んだかを見る）
	bool mHasLastDestination;                     // mLastDestination が入っているか（clear の後は空）
	int mProbeTicks;                              // 次にまっすぐ通れるかを調べるまでの残り（ティック）
	bool mProgressStarted;                        // 近づけているかを数え始めたか（向く点が変わると false）
	float mBestDistance;                          // 今の向く点へ、これまでで一番近づいた距離
	int mStallTicks;                              // mBestDistance が縮まないまま過ぎた長さ（ティック）
};

#endif
