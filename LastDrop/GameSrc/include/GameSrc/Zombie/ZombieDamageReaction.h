#ifndef INCLUDED_ZOMBIE_DAMAGE_REACTION_H
#define INCLUDED_ZOMBIE_DAMAGE_REACTION_H

// ゾンビが撃たれたときに、どう反応するか（倒れる・脚を失って倒れる・よろける）を選ぶ。状態は持たない
// （ZombieAttackReach と同じ形）。選ぶだけで、アニメを流す・状態を変えるのは持ち主（Zombie::applyDamage）。
// 優先順位：倒れる → 脚を失って倒れる → よろける（撃たれた反応の優先順位の修正の入口。
// SummaryPlan/個別仕様/ZombieSplit4_Plan.txt の 9 章。2026-11-01 につかみをやめ、振りほどかれる反応を除いた）。
class ZombieDamageReaction{
public:
	// 撃たれた直後（体力を減らし、部位を切った後）の体の様子。Zombie が作って渡す。
	struct Facts{
		bool canFall;        // 死亡アニメがある（無い確認用の体は倒れない）
		bool isDead;         // 体力が 0
		bool isStanding;     // 立っている（Alive。脚を失って倒れている Downed ではない）
		bool hasLostFooting; // 脚を失った
		bool isAttacking;    // 攻撃の最中（ZombieAttack::isActive）。攻撃のアニメが途切れないよう、よろけない
		bool canStagger;     // よろけてよい（よろけのアニメがある・間あけが済んだ、または頭）
	};
	enum class Fall{
		None,
		Die,  // 死亡アニメで倒れる（Falling へ）
		Down, // 脚を失って倒れ、倒れたまま生きている（Downed へ）
	};
	enum class Hit{
		None,
		Stagger,  // よろける
	};

	// 倒れるか。None のときだけ、持ち主が chooseHit を呼ぶ。
	static Fall chooseFall( const Facts& facts );
	// 倒れないときの反応。
	static Hit chooseHit( const Facts& facts );
};

#endif
