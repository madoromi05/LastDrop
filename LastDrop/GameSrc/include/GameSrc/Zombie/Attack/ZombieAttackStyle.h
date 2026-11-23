#ifndef INCLUDED_ZOMBIE_ATTACK_STYLE_H
#define INCLUDED_ZOMBIE_ATTACK_STYLE_H

// 立っているゾンビの攻撃の種類
// 遊んで比べて替えることが多いので、どちらも消さずに切り替えられる形にしてある
// どちらにするかは ZombieDirector の kZombieAttackStyle の 1 か所で決める（流す攻撃のアニメもそこで一緒に替わる）
// 体を引きずる体は、どちらでも足元へ腕を伸ばして 1 回当てる（つかまない）
enum class ZombieAttackStyle{
	Swing, // 腕を振り下ろして 1 回当てる（atack1。つかまない・空振りあり）
	Grab,  // つかむ → 噛む → 離す（atack3。つかまれている間は動けない・撃って振りほどく）
};

#endif
