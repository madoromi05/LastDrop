#ifndef INCLUDED_BUILDING_H
#define INCLUDED_BUILDING_H

#include "GameLib/Math/Vector3.h"
#include "StaticMeshInstance.h"
#include "WallMesh.h"
#include "IRayBlocker.h"

class StaticMeshData;
class CollisionWorld;

// 建物1軒。見た目（StaticMeshInstance）と移動の当たり判定（WallMesh）を、同じ位置・向きから作って持つ。
// 置き場所を決める入口がここ1か所なので、見た目と当たり判定がずれない。
// 移動の当たり判定は、見た目の三角形を体の高さの範囲で切り取ったもの（MeshWallBuilder）。
// 箱1つで囲むのと違い、へこんだ入口には入れて、斜めの壁には沿って滑る。足元の歩道や頭より上の庇はふさがない。
// 中に入れるモデルに替えれば、そのまま中に入れる（壁の三角形だけがふさぐので、中は空いている）。
// 弾も見た目の三角形そのもので止める（IRayBlocker）。こちらは高さで切らず、全部の三角形が相手。
class Building : public IRayBlocker{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。isReady() 済みであること（当たり判定を三角形から作るため）。
	//       この建物より長く生きていること（MeshCache が持つ）。
	// position / angleY: 置き場所（地面の上）と向き（度）。
	Building( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY );

	// 移動の当たり判定を world へ登録する（world が値をコピーして持つ）。world: 所有権は受け取らない。nullptr 不可。
	void addCollidersTo( CollisionWorld* world ) const;
	void draw() const;
	// 弾の当たり判定（見た目の三角形で判定する）。
	virtual bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;

private:
	StaticMeshInstance mMesh;
	WallMesh mWalls;
};

#endif
