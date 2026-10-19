#ifndef INCLUDED_CHASE_TARGET_H
#define INCLUDED_CHASE_TARGET_H

#include "GameLib/Math/Vector3.h"

// ゾンビが追う先（自キャラ）の、追うのに要る所だけ。持ち主（GameWorld）が毎ティック作って渡す。
// ゾンビに自キャラ（PlayerCharacter）そのものを渡さないのは、ゾンビが自キャラの中身を知らずに済むようにするため。
struct ChaseTarget{
	GameLib::Math::Vector3 position;    // 足元の位置（向かう所）
	GameLib::Math::Vector3 eyePosition; // 目の位置（ここが見えているかで気付く）
};

#endif
