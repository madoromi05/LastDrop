#ifndef INCLUDED_ZOMBIE_PATH_H
#define INCLUDED_ZOMBIE_PATH_H

#include "GameLib/Math/Vector3.h"
#include <vector>

class NavMesh;
class PathBudget;

// ゾンビ1体の通り道（建物を回り込むときだけ使う）。行き先（どこへ行きたいか）は ZombieChase が決め、
// ここは「このティックにどこを向いて歩くか」だけを答える。自分では動かない（動くのは Zombie）
// 決まった間隔で、行き先へ床（NavMesh）の上をまっすぐ通れるかを調べる（体ごとに時刻をずらす）
//   通る  ：通り道を捨てて、行き先へまっすぐ向かう（今までと同じ動き）
//   当たる：通り道が無いか、行き先が前に探したときから大きく動いていたら NavMesh に探し直させる
// 通り道の点に近づいたら次の点へ。次の次の点が見えていれば、今の点を飛ばす（角を早めに斜めに曲がる）
// 向く点の向きへ自分が進めているかも数える（見失った後に着けない所で押し続けないため。あきらめるかは持ち主が決める）
// 点との距離では見ない（逃げる自キャラを追う間は、歩けていても距離が縮まらないため）
// 行き先が 1 ティックで大きく飛んだら（新しい銃声の所へ向かい直した）、すぐ調べ直して数え直す
// 歩いているのに向かう点の向きへ決まった時間進めなければ（群れ・床の形に無い小さな物に詰まった）、左右の通れる方へ少しよけてから
// 向かい直す（SummaryPlan/個別仕様/ゾンビ/ZombieCrowd_Plan.txt の 5 章。値は ZombieCrowdParams）
// 道を探す（findPath）のは PathBudget が許したときだけ。待たされたら前の通り道（無ければ行き先へまっすぐ）のまま歩き、次のティックに頼み直す
class ZombiePath{
public:
	// probePhase: まっすぐ通れるかを調べる時刻をずらす番号（何体目か）。同じティックに全員が線を引かないようにする
	// budget: 1 ティックに道を探してよい回数を配る係。所有権は受け取らない。nullptr 不可。この物より長く生きていること
	ZombiePath( int probePhase, PathBudget* budget );
	~ZombiePath();
	ZombiePath( const ZombiePath& ) = delete;
	ZombiePath& operator=( const ZombiePath& ) = delete;

	// このティックに向く点を返す。self: 自分の足元。destination: 最後に向かいたい所（ZombieChase::destination）
	// walking: 歩こうとしているか（ZombieChase::wantsToWalk）。立ち止まっている間は「近づけない」を数えない
	// 毎ティック呼ぶ（間隔を数え、近づけているかを数えるため）
	const GameLib::Math::Vector3& steer( const GameLib::Math::Vector3& self, const GameLib::Math::Vector3& destination,
		const NavMesh& navMesh, bool walking );
	// 通り道を捨てる（うろつきに戻った・攻撃を始めた）。次の steer ですぐ、まっすぐ通れるかを調べる
	void clear();
	// 一度よけても、まだ向く点へ近づけないか（見失った後、着けない所へ向かっているのを見分ける。あきらめるかは持ち主が決める）
	bool isStuck() const;
	// 道を探したいのに、順番を待っているか（確認用の表示）
	bool isWaitingForSearch() const;
	// 近づけているかを数え直す（探し始めたとき。追っている間に数えた分を持ち越さない）
	void resetProgress();
	// 残りの通り道（今向かっている点から最後＝行き先まで）。まっすぐ向かっている間は空。よけている間は、よける点が先頭に入る
	// 確認用の表示に使う。out: nullptr 不可。前の中身は捨てる
	void remainingRoute( std::vector< GameLib::Math::Vector3 >* out ) const;

private:
	// 詰まった。point（今向かっている点）への向きの左右のうち、まっすぐ通れる方（両方なら point に近い方）へよける点を決める
	// どちらも通れなければ、少し後ろへ下がる点にする
	void startSidestep( const GameLib::Math::Vector3& self, const GameLib::Math::Vector3& point,
		const NavMesh& navMesh );
	// 近づけているかを数え直す（よけた回数・詰まった印は残す。よけ終わった後に使う）
	void restartProgress();
	// 決まった間隔で、まっすぐ通れるか・探し直すか・点を飛ばせるかを決める
	void probe( const GameLib::Math::Vector3& self, const GameLib::Math::Vector3& destination,
		const NavMesh& navMesh );
	// 今向かっている点（通り道が無ければ行き先）
	const GameLib::Math::Vector3& currentPoint( const GameLib::Math::Vector3& destination ) const;

	PathBudget* mBudget;                          // 借りているだけ
	std::vector< GameLib::Math::Vector3 > mRoute; // 通り道（最後は行き先）。空ならまっすぐ向かう
	int mNext;                                    // mRoute のうち今向かっている点の番号
	GameLib::Math::Vector3 mRouteGoal;            // 通り道を探したときの行き先（大きく動いたら探し直す）
	GameLib::Math::Vector3 mLastDestination;      // 前のティックの行き先（飛んだかを見る）
	bool mHasLastDestination;                     // mLastDestination が入っているか（clear の後は空）
	int mProbeTicks;                              // 次にまっすぐ通れるかを調べるまでの残り（ティック）
	bool mProgressStarted;                        // 近づけているかを数え始めたか（向く点が変わると false）
	GameLib::Math::Vector3 mProgressOrigin;       // 最後に「進めている」とみなした時の自分の足元
	int mStallTicks;                              // 向く点の向きへ進めないまま過ぎた長さ（ティック）
	GameLib::Math::Vector3 mSidestepPoint;        // よける点
	int mSidestepTicks;                           // よける点へ向かう残り（ティック。0 ならよけていない）
	int mSidestepCount;                           // 今の向く点へ向かう間によけた回数
	bool mStuck;                                  // よけた後も、また詰まったか（isStuck）
};

#endif
