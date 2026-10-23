#ifndef INCLUDED_GROUND_MATERIAL_H
#define INCLUDED_GROUND_MATERIAL_H

#include "GameLib/Math/Vector3.h"
#include <vector>

// 床の材質。足音の音色を決める。
enum GroundMaterial{
	GROUND_CONCRETE,
	GROUND_METAL,
	GROUND_WOOD,
	GROUND_GRASS,
	GROUND_GRAVEL,
	GROUND_WATER,	// 水たまり
	GROUND_GLASS,	// 割れたガラスが散らばった床
	GROUND_NUM,
};

// 地面の上に置いた長方形（上から見た XZ の範囲）の材質。
struct GroundZone{
	float minX, minZ;
	float maxX, maxZ;
	GroundMaterial material;
};

class IGroundSurface;

// 位置からその場所の床の材質を引く。
// 下地は地面（IGroundSurface）に聞き、その上に長方形（GroundZone）を重ねる。
// 長方形は、地面の部品からは出てこない材質（水たまり・ガラスなど）を置くためのもので、地面より優先する。
// 足音の係は materialAt しか呼ばないので、材質の決め方を変えてもこのクラスより外へ波及しない。
class GroundMaterialMap{
public:
	// zones: 長方形の並び。重なっていたら先の方を使う。
	// surface: 長方形の外の材質を答える地面。所有権は受け取らない。nullptr 不可。この表より長く生きていること。
	// fallback: 地面も答えられない場所（地面の外・地面を並べる前）の材質。
	GroundMaterialMap( const GroundZone* zones, int zoneCount, const IGroundSurface* surface, GroundMaterial fallback );
	GroundMaterial materialAt( const GameLib::Math::Vector3& position ) const;
private:
	std::vector< GroundZone > mZones;
	const IGroundSurface* mSurface; // 借りているだけ
	GroundMaterial mFallback;
};

#endif
