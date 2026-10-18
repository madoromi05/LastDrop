#ifndef INCLUDED_AXIS_ROTATION_H
#define INCLUDED_AXIS_ROTATION_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"

// 好きな軸のまわりに回す行列を作る、ただ1か所。状態は持たない。
// GameLib の Matrix34 には X/Y/Z 軸まわりの回転しか無いので、任意の軸はここで組む（ロドリゲスの回転公式）。
// 骨を関節で曲げる処理（一人称の体の補正・ゾンビののけぞり）が同じ式を使う。
class AxisRotation{
public:
	// pivot を中心に、長さ1の軸 axis のまわりへ degree 度回す行列。
	static GameLib::Math::Matrix34 about( const GameLib::Math::Vector3& pivot, const GameLib::Math::Vector3& axis, float degree );
};

#endif
