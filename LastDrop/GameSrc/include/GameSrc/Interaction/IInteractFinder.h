#ifndef INCLUDED_I_INTERACT_FINDER_H
#define INCLUDED_I_INTERACT_FINDER_H

#include "GameLib/Math/Vector3.h"

class IInteractable;
class IHittable;

// 視線の先の「使える物」を探す（SummaryPlan/個別仕様/設計と分割/InteractorComponent_Plan.txt）
// 実装するのは世界の係（InteractionSystem）。世界が共有の棚へ置き、自キャラの InteractorComponent が引く
// 見る側の部品は、どの名簿をどう調べるか（壁の向こうは見えない・落ちている物は一回り大きい箱で見る）を知らずに済む
class IInteractFinder{
public:
	virtual ~IInteractFinder(){}
	// origin から delta だけ進む視線の先の、一番手前の使える物。無ければ nullptr
	// viewer: 視線を当てない的（目が体の中にある、見ている本人の体）。nullptr 可
	virtual IInteractable* findInteractable( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta, const IHittable* viewer ) = 0;
};

#endif
