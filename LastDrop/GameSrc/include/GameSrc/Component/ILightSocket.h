#ifndef INCLUDED_I_LIGHT_SOCKET_H
#define INCLUDED_I_LIGHT_SOCKET_H

#include "GameLib/Math/Vector3.h"

// 光の出る所の今の位置と向きを答える（SummaryPlan/個別仕様/設計と分割/LightComponent_Plan.txt）
// 置き場所を持たない物の光（手に持つ銃の懐中電灯・銃口の光）へ、持つ側が自分を渡す
// 光の部品は持つ側のクラスを知らずに済む（銃から持つ側への依存を作らない）
class ILightSocket{
public:
	virtual ~ILightSocket(){}
	// 今の位置（ワールド）と向き（長さ1）。今は光の出る所が無ければ false（out は変えない）。どちらも nullptr 不可
	virtual bool lightSocket( GameLib::Math::Vector3* position, GameLib::Math::Vector3* direction ) const = 0;
};

#endif
