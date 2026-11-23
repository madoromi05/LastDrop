#ifndef INCLUDED_ZOMBIE_ATTACK_H
#define INCLUDED_ZOMBIE_ATTACK_H

// ゾンビ1体の攻撃の段階を決める。自分では動かず、アニメも持たない
// （ZombieChase と同じ立場。攻撃のアニメを流し、向きを変え、寄るのは ZombieAttackComponent）
// 立っている体は、攻撃の種類（ZombieAttackStyle）で 2 通り。段階はアニメの時刻（フレーム）で進める
//   腕振り（startSwing ＋ update。atack1）：Swing → Recover。体を引きずる体と同じ時間割
//   つかみ（start ＋ update。atack3）　　 ：Reach → Hold → Recover（下の通り）
// アニメを正とするので、アニメの見た目と「つかんだ・噛んだ」がずれない
//   Ready   ：攻撃していない（終わってから少しの間は、次の攻撃を始めない）
//   Reach   ：両手を伸ばしている。伸ばし切ったとき、つかめる所に相手が居ればつかむ（Hold）、居なければ空振り（Recover）
//   Hold    ：つかんでいる（アニメをゆっくり流す。この間が相手の逃げる時間）。決まった時刻まで来たら噛んで Recover
//             この間に決まった量のダメージを受けると振りほどかれ、すぐ Ready
//   Recover ：噛んだ後・空振りの後、アニメの終わりまで流して元の姿勢へ戻る
// 腕振りは 1 回当てるだけ（startSwing。つかまない）。立っている体は atack1 の時刻で、
// 体を引きずる体はアニメの代わりに自分の時計で進める（updateWithoutAnimation）
//   Swing   ：腕を振っている。決まった時刻に、届く所に相手が居れば当たる（居なければ空振り）。その後 Recover
class ZombieAttack{
public:
	enum class Phase{
		Ready,
		Reach,
		Hold,
		Swing,
		Recover,
	};

	ZombieAttack();

	bool canStart() const;	// 攻撃を始めてよいか（攻撃していない・前の攻撃から待ち時間が過ぎた）。間合い・向きは呼ぶ側が見る
	void start();			// 立っている体が、つかみに行く（Reach）。呼ぶ側は、攻撃のアニメを頭から PLAY_ONCE で流し始め、毎ティック update を呼ぶ
	// 腕を振る（Swing）。立っている体は、呼ぶ側が腕振りのアニメ（atack1）を頭から PLAY_ONCE で流し始め、毎ティック update を呼ぶ
	// 体を引きずる体は、呼ぶ側はアニメを流さず、毎ティック updateWithoutAnimation を呼ぶ
	void startSwing();
	// 1ティック進める（立っている体）。攻撃していなくても毎ティック呼ぶ（待ち時間を数える）
	// animationTime: 攻撃のアニメの今見えている時刻（フレーム。SkinnedMeshInstance::animationTime）。攻撃していない間は使わない
	// animationFinished: 攻撃のアニメが最後まで来たか
	// targetInReach: 今つかめる所に相手が居るか（手を伸ばし切ったときだけ使う）
	void update( float animationTime, bool animationFinished, bool targetInReach );
	// update と同じだが、攻撃のアニメを流さない体（体を引きずるゾンビ）のために、アニメの代わりに自分の時計で段階を進める
	// 時計は「見えている時刻」を渡してから 1 フレーム進める（アニメと同じく 1 ティックに 1 フレーム）
	// targetInReach: 今腕が届く所に相手が居るか（当たる時刻だけ使う）
	void updateWithoutAnimation( bool targetInReach );
	// つかんでいる間に受けたダメージ（部位の倍率を掛けた後）を数える。つかんでいなければ何もしない
	// 合計が決まった量に届いたら振りほどかれて Ready になり、true を返す（呼ぶ側が、よろけのアニメなどへ替える）
	bool takeShakeDamage( float amount );
	void cancel();			// 攻撃をやめる（倒れた・脚を失った）。待ち時間を付けて Ready にする。攻撃していなければ何もしない

	Phase phase() const;
	bool isActive() const;			// 攻撃の途中（Ready 以外）
	bool isAttacking() const;		// 相手を取りに行っている・つかんでいる・腕を伸ばしている間（Reach・Hold・Swing）。この間は、ほかのゾンビが攻撃を始めない
	bool isHolding() const;			// つかんでいる間（Hold）
	bool grabbedThisTick() const;	// 直前の update でつかんだか
	bool hitThisTick() const;		// 直前の update で相手にダメージを与えたか（立っている体は噛んだ、体を引きずる体は腕が当たった）。量は hitDamage
	float playbackRate() const;		// 攻撃のアニメの再生の速さ（SkinnedMeshInstance::setPlaybackRate へ渡す）。つかんでいる間だけ遅い

	static float hitDamage();	// 1回噛んだ・腕が当たったときのダメージ
	static float startTime();	// 攻撃のアニメを流し始める時刻（フレーム）。段階の時刻はこのアニメの時刻なので、流す側もここから流す

private:
	void finish(); // 待ち時間を付けて Ready にする

	Phase mPhase;
	int mCooldownTicks;   // 次の攻撃を始められるまでの残り（ティック。Ready の間だけ減る）
	float mShakeDamage;   // つかんでいる間に受けたダメージの合計
	float mClock;         // updateWithoutAnimation で進める、アニメの代わりの時刻（フレーム）
	bool mGrabbedThisTick;
	bool mHitThisTick;
};

#endif
