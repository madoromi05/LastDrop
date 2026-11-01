#ifndef INCLUDED_CHASE_TARGET_H
#define INCLUDED_CHASE_TARGET_H

#include "GameLib/Math/Vector3.h"

class ICollisionBody;

// ゾンビが追う先（自キャラ）の、追うのに要る所だけ。持ち主（GameWorld）が毎ティック作って渡す。
// ゾンビに自キャラ（PlayerCharacter）そのものを渡さないのは、ゾンビが自キャラの中身を知らずに済むようにするため。
struct ChaseTarget{
	GameLib::Math::Vector3 position;    // 足元の位置（向かう所）
	GameLib::Math::Vector3 eyePosition; // 目の位置（ここが見えているかで気付く）
	// 攻撃してよいか（生きている。自キャラの決まり）。
	// false の間は攻撃を始めず、腕を振り下ろしたときも空振りにする。
	bool attackable;
	// ほかのゾンビが、もう攻撃に行っているか（腕を振っている）。true の間は攻撃を始めない（一度に 1 体だけ）。
	bool attackTaken;
	// 移動の当たり判定（自キャラの箱）。攻撃の踏み込み（根の動き）は、これを通り抜ける（止めると手が届かないため。
	// SummaryPlan/個別仕様/ZombieRootMotion_Plan.txt の Q1）。借りるだけ。このティックの update の間だけ使う（覚えない）。
	const ICollisionBody* body;
};

#endif
