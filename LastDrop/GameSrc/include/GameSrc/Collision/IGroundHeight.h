#ifndef INCLUDED_IGROUND_HEIGHT_H
#define INCLUDED_IGROUND_HEIGHT_H

#include "GameLib/Math/Vector3.h"

// キャラが乗る地面の高さを答えられる物（町の地面など）
// CollisionWorld に登録すると、CharacterMover がキャラの足元の高さを合わせるのに使う（歩道の段差に乗る）
class IGroundHeight{
public:
	virtual ~IGroundHeight(){}
	// position（キャラの足元。ワールド座標）の真下の面の高さを height に書いて true
	// 足元から MoveHeight::kStepOver より上の面（歩いて越えられない高さ）は探さない。足元に面が無ければ false
	// height: 出力先。nullptr 不可
	virtual bool heightBelow( const GameLib::Math::Vector3& position, float* height ) const = 0;
};

#endif
