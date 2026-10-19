#ifndef INCLUDED_ZOMBIE_CHASE_H
#define INCLUDED_ZOMBIE_CHASE_H

#include "GameLib/Math/Vector3.h"

struct ChaseTarget;
class HitWorld;

// ゾンビ1体の「どこへ行きたいか」を決める（追ってくる AI の頭の部分）。自分では動かない（動くのは Zombie）。
//   Idle   ：気付いていない。その場に立つ
//   Chase  ：自キャラが見えている。自キャラへ向かい、近づいたら止まってこちらを向く
//   Search ：見失った。最後に見た場所へ向かい、着いたら Idle に戻る
// 気付くのは「近くで見えている（間に壁・建物が無い。向きは問わない）」か「撃たれた」とき（alert）。
// 見えているかの線は決まった間隔でしか引かない（地面の判定は重いので）。体ごとに引く時刻をずらす。
// 建物は回り込まない（行き先へまっすぐ向かうだけ。壁に沿って滑るのは CharacterMover）。
// 値の理由は SummaryPlan/個別仕様/ZombieChase_Plan.txt。
class ZombieChase{
public:
	enum class Mode{
		Idle,
		Chase,
		Search,
	};

	// sightPhase: 見通しの線を引く時刻をずらす番号（何体目か）。同じティックに全員が線を引かないようにする。
	explicit ZombieChase( int sightPhase );

	// 撃たれた。次の update で、見えていなくても自キャラの居る所へ向かい始める。
	void alert();
	// 1ティック考える。self / eye: 自分の足元と目の位置（ワールド）。
	// target: 追う先。nullptr なら追う相手が居ない（気付いていても Idle へ戻る）。
	// hitWorld: 見通しを聞く名簿（弾を止める物だけを見る）。
	void update( const GameLib::Math::Vector3& self, const GameLib::Math::Vector3& eye, const ChaseTarget* target,
		const HitWorld& hitWorld );

	Mode mode() const;
	// 向かう所があるか（Chase・Search）。あれば destination の方を向く。
	bool hasDestination() const;
	// 向かう所（Chase なら自キャラの足元、Search なら最後に見た場所）。hasDestination が false なら意味が無い。
	const GameLib::Math::Vector3& destination() const;
	// 歩いて近づくか（向かう所があって、まだ着いていない）。false なら立ったまま向きだけ変える。
	bool wantsToWalk() const;

private:
	// 自分の目から target の目までが、近くて間に何も無いか。
	bool canSee( const GameLib::Math::Vector3& eye, const ChaseTarget& target, const HitWorld& hitWorld ) const;
	// 今の位置から、歩くか止まるかを決め直す（止まる距離と歩き出す距離を分けて、境目でふらつかないようにする）。
	void updateWalking( const GameLib::Math::Vector3& self );

	Mode mMode;
	GameLib::Math::Vector3 mDestination;
	bool mWalking;
	bool mSeesTarget;  // 最後に線を引いたときに見えていたか
	bool mAlerted;     // 撃たれて、まだ update で受け取っていない
	int mSightTicks;   // 次に見通しの線を引くまでの残り（ティック）
};

#endif
