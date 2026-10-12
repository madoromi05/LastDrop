#ifndef INCLUDED_IGROUND_SURFACE_H
#define INCLUDED_IGROUND_SURFACE_H

#include "GroundMaterial.h"
#include "GameLib/Math/Vector3.h"

// 足元の材質を答えられる地面（町の地面など）。
// GroundMaterialMap が、長方形で上書きしていない場所の材質をここへ聞く。
class IGroundSurface{
public:
	virtual ~IGroundSurface(){}
	// position（キャラの足元。ワールド座標）が乗っている面の材質を material に書いて true。足元に面が無ければ false。
	// material: 出力先。nullptr 不可。
	virtual bool materialBelow( const GameLib::Math::Vector3& position, GroundMaterial* material ) const = 0;
};

#endif
