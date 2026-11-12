#ifndef INCLUDED_ZOMBIE_CHASE_H
#define INCLUDED_ZOMBIE_CHASE_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Random.h"

struct ChaseTarget;
class HitWorld;

// ゾンビ1体の「どこへ行きたいか」を決める（追ってくる AI の頭の部分）。自分では動かない（動くのは Zombie）
//   Wander ：気付いていない。立つ ⇔ 近くをうろつく を繰り返す（うろつく先は今いる所と今の向きから決める）
//   Chase  ：自キャラが見えている。自キャラへ向かい、近づいたら止まってこちらを向く
//   Search ：見失った。最後に見た場所へ向かい、着いたら Wander に戻る（まず立つ）
// 気付くのは「見えている（間に壁・建物が無い）」か「撃たれた」とき（alert）
// 銃声を聞いたら（hearNoise）、体ごとに少し遅れてから、聞いた所へ探しに行く（Search。追っている間は聞かない
// SummaryPlan/個別仕様/ゾンビ/ZombieHearing_Plan.txt）
// 見えているかの線は決まった間隔でしか引かない（地面の判定は重いので）。体ごとに引く時刻をずらす
// ここが決めるのは最後に向かいたい所だけ。建物を回り込む通り道は ZombiePath が決める（Chase・Search の間だけ）
class ZombieChase{
public:
	enum class Mode{
		Wander,
		Chase,
		Search,
	};

	// sightPhase: 見通しの線を引く時刻をずらす番号（何体目か）。同じティックに全員が線を引かないようにする
	// randomSeed: うろつく先・立つ長さを選ぶ乱数の種（体ごとに違う値を渡す）
	ZombieChase( int sightPhase, int randomSeed );

	// 撃たれた。次の update で、見えていなくても自キャラの居る所へ向かい始める
	void alert();
	// 見失った後（Search）、最後に見た場所へ着けない（近づけない）。探すのをやめて、その場で立ってからうろつく
	// Search でなければ何もしない
	void giveUpSearch();
	// position で銃声がした（届くかは知らせる側が決める）。追っていなければ、少し遅れてからそこへ探しに行く
	// 遅れを待っている間に次が来たら、向かう先だけ替える（待つ長さは延ばさない）
	void hearNoise( const GameLib::Math::Vector3& position );
	// 1ティック考える。self / eye: 自分の足元と目の位置（ワールド）。angleY: 自分の向き（度。0 は +Z）
	// target: 追う先。nullptr なら追う相手が居ない（気付いていてもうろつきに戻る）
	// hitWorld: 見通しを聞く名簿（弾を止める物だけを見る）
	void update( const GameLib::Math::Vector3& self, const GameLib::Math::Vector3& eye, float angleY,
		const ChaseTarget* target, const HitWorld& hitWorld );

	Mode mode() const;
	// 向かう所があるか（Chase・Search と、うろついて歩いている間）。あれば destination の方を向く
	bool hasDestination() const;
	// 向かう所（Chase なら自キャラの足元、Search なら最後に見た場所、Wander ならうろつく先）
	// hasDestination が false なら意味が無い
	const GameLib::Math::Vector3& destination() const;
	// 歩いて近づくか（向かう所があって、まだ着いていない）。false なら立ったまま向きだけ変える
	bool wantsToWalk() const;

private:
	// 見えているか（mSeesTarget）を決め直す。撃たれていればすぐ、そうでなければ決まった間隔で線を引く
	void updateSight( const GameLib::Math::Vector3& eye, float angleY, const ChaseTarget* target, const HitWorld& hitWorld );
	// 自分の目から target の目までが、見える距離の内で間に何も無いか
	// 見える距離は、追っている間は向き問わず遠く、それ以外は前だけ遠く・横と後ろは近く
	bool canSee( const GameLib::Math::Vector3& eye, float angleY, const ChaseTarget& target, const HitWorld& hitWorld ) const;
	// 止まる距離と歩き出す距離を分けて、境目でふらつかないように歩くか決める
	void updateChase( const GameLib::Math::Vector3& self );
	// 聞いた銃声の遅れを数え、待ち終えたら聞いた所へ探しに行く（見えていないときだけ呼ぶ）
	void updateNoise();
	// 最後に見た場所に着いたら、うろつきへ戻る
	void updateSearch( const GameLib::Math::Vector3& self );
	// 立つ時間を数え、終わったらうろつく先を選ぶ。歩いている間は、着いたか・近づけているかを見る
	void updateWander( const GameLib::Math::Vector3& self, float angleY, const HitWorld& hitWorld );
	// うろつきの「立つ」を始める（立つ長さは毎回ランダム）
	void startWanderWait();
	// うろつく先を選んで mDestination へ書く。腰の高さの線が壁・建物・車に当たらない先だけ選ぶ
	// 何度選んでも当たるなら false
	bool pickWanderPoint( const GameLib::Math::Vector3& self, float angleY, const HitWorld& hitWorld );

	Mode mMode;
	GameLib::Math::Vector3 mDestination;
	GameLib::Math::Random mRandom;
	bool mWalking;
	bool mSeesTarget;  // 最後に線を引いたときに見えていたか
	bool mAlerted;     // 撃たれて、まだ update で受け取っていない
	int mSightTicks;   // 次に見通しの線を引くまでの残り（ティック）
	// ── 聞いた銃声（hearNoise から、向かい始めるまで）──
	bool mNoisePending;               // 聞いて、まだ向かい始めていない
	GameLib::Math::Vector3 mNoisePosition; // 最後に聞いた所
	int mNoiseDelayTicks;             // 向かい始めるまでの残り（ティック）
	// ── うろつき（Wander の間だけ使う）──
	int mWanderWaitTicks;     // 立っている残り（ティック）
	float mWanderBestDistance; // 今のうろつく先へ、これまでで一番近づいた距離（近づけているかを見る）
	int mWanderStallTicks;    // mWanderBestDistance が縮まないまま過ぎた長さ（ティック）
	bool mWanderTurnBack;     // 前に選べなかったので、次は後ろ向きを基準に選ぶ
};

#endif
