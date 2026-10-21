#ifndef INCLUDED_ZOMBIE_ATTACK_H
#define INCLUDED_ZOMBIE_ATTACK_H

// ゾンビ1体の攻撃（つかむ → 噛む → 離す）の段階を決める。自分では動かず、アニメも持たない
// （ZombieChase と同じ立場。攻撃のアニメ atack3 を流し、向きを変え、寄るのは Zombie）。
// 段階はアニメの時刻（フレーム）で進める。アニメを正とするので、アニメの見た目と「つかんだ・噛んだ」がずれない。
//   Ready   ：攻撃していない（終わってから少しの間は、次の攻撃を始めない）
//   Reach   ：両手を伸ばしている。伸ばし切ったとき、つかめる所に相手が居ればつかむ（Hold）、居なければ空振り（Recover）
//   Hold    ：つかんでいる（アニメをゆっくり流す。この間が相手の逃げる時間）。決まった時刻まで来たら噛んで Recover。
//             この間に決まった量のダメージを受けると振りほどかれ、すぐ Ready
//   Recover ：噛んだ後・空振りの後、アニメの終わりまで流して元の姿勢へ戻る
// 値の理由は SummaryPlan/個別仕様/ZombieAttack_Plan.txt の 3 章。
class ZombieAttack{
public:
	enum class Phase{
		Ready,
		Reach,
		Hold,
		Recover,
	};

	ZombieAttack();

	// 攻撃を始めてよいか（攻撃していない・前の攻撃から待ち時間が過ぎた）。間合い・向きは呼ぶ側が見る。
	bool canStart() const;
	// 攻撃を始める（Reach）。呼ぶ側は、攻撃のアニメを頭から PLAY_ONCE で流し始める。
	void start();
	// 1ティック進める。攻撃していなくても毎ティック呼ぶ（待ち時間を数える）。
	// animationTime: 攻撃のアニメの今見えている時刻（フレーム。SkinnedMeshInstance::animationTime）。攻撃していない間は使わない。
	// animationFinished: 攻撃のアニメが最後まで来たか。
	// targetInReach: 今つかめる所に相手が居るか（手を伸ばし切ったときだけ使う）。
	void update( float animationTime, bool animationFinished, bool targetInReach );
	// つかんでいる間に受けたダメージ（部位の倍率を掛けた後）を数える。つかんでいなければ何もしない。
	// 合計が決まった量に届いたら振りほどかれて Ready になり、true を返す（呼ぶ側が、よろけのアニメなどへ替える）。
	bool takeShakeDamage( float amount );
	// 攻撃をやめる（倒れた・脚を失った）。待ち時間を付けて Ready にする。攻撃していなければ何もしない。
	void cancel();

	Phase phase() const;
	bool isActive() const;   // 攻撃のアニメを流している間（Reach・Hold・Recover）
	bool isAttacking() const; // 相手を取りに行っている・つかんでいる間（Reach・Hold）。この間は、ほかのゾンビが攻撃を始めない
	bool isHolding() const;  // つかんでいる間（Hold）
	bool grabbedThisTick() const; // 直前の update でつかんだか
	bool bitThisTick() const;     // 直前の update で噛んだか
	// 攻撃のアニメの再生の速さ（SkinnedMeshInstance::setPlaybackRate へ渡す）。つかんでいる間だけ遅い。
	float playbackRate() const;

	// 1回噛んだときのダメージ。
	static float biteDamage();

private:
	void finish(); // 待ち時間を付けて Ready にする

	Phase mPhase;
	int mCooldownTicks;    // 次の攻撃を始められるまでの残り（ティック。Ready の間だけ減る）
	float mShakeDamage;    // つかんでいる間に受けたダメージの合計
	bool mGrabbedThisTick;
	bool mBitThisTick;
};

#endif
