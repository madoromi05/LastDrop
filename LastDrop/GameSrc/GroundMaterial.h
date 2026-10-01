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

// デバッグ表示用の材質の名前。
const char* groundMaterialName( GroundMaterial );

// 地面の上に置いた長方形（上から見た XZ の範囲）の材質。
struct GroundZone{
	float minX, minZ;
	float maxX, maxZ;
	GroundMaterial material;
};

// 位置からその場所の床の材質を引く。
// 今は地面に長方形を並べるだけの簡易版（ステージに材質の情報がまだ無いため）。
// ステージのポリゴンに材質を持たせるようになったら、このクラスの中身だけを「足元へのレイの当たったポリゴンの材質」に替える
// （足音の係は materialAt しか呼ばないので波及しない）。
class GroundMaterialMap{
public:
	// zones: 長方形の並び。重なっていたら先の方を使う。fallback: どの長方形にも入らない場所の材質。
	GroundMaterialMap( const GroundZone* zones, int zoneCount, GroundMaterial fallback );
	GroundMaterial materialAt( const GameLib::Math::Vector3& position ) const;
private:
	std::vector< GroundZone > mZones;
	GroundMaterial mFallback;
};

#endif
