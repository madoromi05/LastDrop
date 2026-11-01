#ifndef INCLUDED_ZOMBIE_ATTACK_H
#define INCLUDED_ZOMBIE_ATTACK_H

// ゾンビ1体の攻撃（右手で引っかく）の段階を決める。自分では動かず、アニメも持たない
// （ZombieChase と同じ立場。攻撃のアニメ atack1 を流し、向きを変え、寄るのは Zombie）。
// 段階はアニメの時刻（フレーム）で進める。アニメを正とするので、アニメの見た目と「当たった」がずれない。
//   Ready   ：攻撃していない（終わってから少しの間は、次の攻撃を始めない）
//   Swing   ：腕を振り上げて振り下ろしている。振り下ろした時刻に、届く所に相手が居れば当たる（居なければ空振り）
//   Recover ：振り終えた後、アニメの終わりまで流して元の姿勢へ戻る
// 値の理由は SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 8 章（2026-11-01 につかみ・噛みから引っかきへ変えた）。
class ZombieAttack{
public:
	enum class Phase{
		Ready,
		Swing,
		Recover,
	};

	ZombieAttack();

	// 攻撃を始めてよいか（攻撃していない・前の攻撃から待ち時間が過ぎた）。間合い・向きは呼ぶ側が見る。
	bool canStart() const;
	// 攻撃を始める（Swing）。呼ぶ側は、攻撃のアニメを頭から PLAY_ONCE で流し始める。
	void start();
	// 1ティック進める。攻撃していなくても毎ティック呼ぶ（待ち時間を数える）。
	// animationTime: 攻撃のアニメの今見えている時刻（フレーム。SkinnedMeshInstance::animationTime）。攻撃していない間は使わない。
	// animationFinished: 攻撃のアニメが最後まで来たか。
	// targetInReach: 今腕が届く所に相手が居るか（振り下ろしたときだけ使う）。
	void update( float animationTime, bool animationFinished, bool targetInReach );
	// update と同じだが、攻撃のアニメを流さない体（体を引きずるゾンビ）のために、アニメの代わりに自分の時計で段階を進める。
	// 時計はアニメと同じく「見えている時刻」を渡してから、1 フレーム進める（アニメは 1 ティックに 1 フレーム進むので同じ時間割になる）。
	// animationPeriod: 攻撃のアニメの長さ（フレーム）。時計がここまで来たら、アニメが最後まで来たとみなす。
	void updateWithoutAnimation( float animationPeriod, bool targetInReach );
	// 攻撃をやめる（倒れた・脚を失った）。待ち時間を付けて Ready にする。攻撃していなければ何もしない。
	void cancel();

	Phase phase() const;
	bool isActive() const;    // 攻撃のアニメを流している間（Swing・Recover）
	bool isAttacking() const; // 腕を振っている間（Swing）。この間は、ほかのゾンビが攻撃を始めない
	bool hitThisTick() const; // 直前の update で相手に当たったか

	// 1回当たったときのダメージ。
	static float hitDamage();
	// 攻撃のアニメを流し始める時刻（フレーム）。段階の時刻はこのアニメの時刻なので、流す側もここから流す。
	static float startTime();

private:
	void finish(); // 待ち時間を付けて Ready にする

	Phase mPhase;
	int mCooldownTicks; // 次の攻撃を始められるまでの残り（ティック。Ready の間だけ減る）
	float mClock;       // updateWithoutAnimation で進める、アニメの代わりの時刻（フレーム）
	bool mHitThisTick;
};

#endif
