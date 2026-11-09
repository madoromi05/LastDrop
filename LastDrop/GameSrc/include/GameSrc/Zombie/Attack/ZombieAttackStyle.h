#ifndef INCLUDED_ZOMBIE_ATTACK_STYLE_H
#define INCLUDED_ZOMBIE_ATTACK_STYLE_H

// 立っているゾンビの攻撃の種類（SummaryPlan/個別仕様/ゾンビ/ZombieAttack_Plan.txt の 11 章）。
// 2026-11-01 腕振り → 11-05 つかみ → 11-09 腕振り、と 3 回替えたので、どちらも消さずに切り替えられる形にした。
// どちらにするかは ZombieDirector の kZombieAttackStyle の 1 か所で決める（流す攻撃のアニメもそこで一緒に替わる）。
// 体を引きずる体は、どちらでも足元へ腕を伸ばして 1 回当てる（つかまない）。
enum class ZombieAttackStyle{
	Swing, // 腕を振り下ろして 1 回当てる（atack1。つかまない・空振りあり。8 章・11 章）
	Grab,  // つかむ → 噛む → 離す（atack3。つかまれている間は動けない・撃って振りほどく。9 章・10 章）
};

#endif
