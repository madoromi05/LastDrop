#ifndef INCLUDED_ZOMBIE_CORPSE_H
#define INCLUDED_ZOMBIE_CORPSE_H

// 倒れ終わったゾンビが、死体として残り、地面へ沈んで消えるまでの時間（ゾンビ1体ぶん）。
// 倒れ終わってから決まった時間残し、決まった時間かけて沈め、沈み終わったら消えてよい（SummaryPlan/個別仕様/ゾンビ/ZombieDamage_Plan.txt）。
// 時計は倒れ終わってからの 1 本だけ（沈み始め・沈む量・消える時刻・残り秒をすべてここから求める）。
// 状態（Corpse → Sinking）を切り替える・見た目の位置を下げるのは持ち主（Zombie）。ここは時間と、どれだけ沈めるかだけ。
class ZombieCorpse{
public:
	ZombieCorpse();

	void start(); // 倒れ終わった。ここから数える
	void tick();  // 1 ティック進める
	bool isSinking() const;         // 残す時間が過ぎて、沈む時間に入ったか
	float sinkDepth() const;        // 今どれだけ沈めるか（m）。沈む時間の前は 0、沈み終わったら一番深い所で止まる
	bool isGone() const;            // 沈み終わって、消してよいか
	float secondsUntilGone() const; // 消えるまでの残り秒（確認用の表示）

private:
	int mTicks; // 倒れ終わってからのティック数
};

#endif
