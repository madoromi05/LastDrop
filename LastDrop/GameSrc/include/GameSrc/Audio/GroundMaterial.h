#ifndef INCLUDED_GROUND_MATERIAL_H
#define INCLUDED_GROUND_MATERIAL_H

#include "GameLib/Math/Vector3.h"

// 床の材質。足音の音色を決める
enum GroundMaterial{
	GROUND_CONCRETE,
	GROUND_METAL,
	GROUND_WOOD,
	GROUND_GRAVEL,
	GROUND_NUM,
};

class IGroundSurface;

// 位置からその場所の床の材質を引く。材質は地面（IGroundSurface）に聞く
// 足音の係は materialAt しか呼ばないので、材質の決め方を変えてもこのクラスより外へ波及しない
class GroundMaterialMap{
public:
	// surface: 材質を答える地面。所有権は受け取らない。nullptr 不可。この表より長く生きていること
	// fallback: 地面が答えられない場所（地面の外・地面を並べる前）の材質
	GroundMaterialMap( const IGroundSurface* surface, GroundMaterial fallback );
	GroundMaterial materialAt( const GameLib::Math::Vector3& position ) const;
private:
	const IGroundSurface* mSurface; // 借りているだけ
	GroundMaterial mFallback;
};

#endif
