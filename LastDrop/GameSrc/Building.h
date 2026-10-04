#ifndef INCLUDED_BUILDING_H
#define INCLUDED_BUILDING_H

#include "GameLib/Math/Vector3.h"
#include "StaticMeshInstance.h"
#include "BoxCollider.h"
#include <vector>

class StaticMeshData;
class CollisionWorld;

// 建物1軒。見た目（StaticMeshInstance）と当たり判定の箱（BoxCollider）を、同じ位置・向きから作って持つ。
// 置き場所を決める入口がここ1か所なので、見た目と当たり判定がずれない。
// 今は、建物全体を真上から見て囲む箱1つ（中には入れない）。中に入れる建物にするときは、
// 箱の作り方（コンストラクタ）だけを壁ごとの箱に差し替える（CollisionWorld・CharacterMover は変えなくてよい）。
class Building{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。isReady() 済みであること（箱の大きさを頂点から求めるため）。
	//       この建物より長く生きていること（MeshCache が持つ）。
	// position / angleY: 置き場所（地面の上）と向き（度）。
	Building( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY );

	// 当たり判定の箱を world へ登録する（world が値をコピーして持つ）。world: 所有権は受け取らない。nullptr 不可。
	void addCollidersTo( CollisionWorld* world ) const;
	void draw() const;

private:
	StaticMeshInstance mMesh;
	std::vector< BoxCollider > mColliders;
};

#endif
