#ifndef INCLUDED_STATIC_PROP_H
#define INCLUDED_STATIC_PROP_H

#include "GameLib/Math/Vector3.h"
#include "StaticMeshInstance.h"
#include "BoxCollider.h"
#include "IRayBlocker.h"

class StaticMeshData;
class CollisionWorld;

// 動かない置物1つ（車・ガソリン缶など）。見た目（StaticMeshInstance）と、頂点を囲む当たり判定の箱1つを、同じ置き場所・向きから作って持つ。
// 弾は見た目の三角形そのもので止める（IRayBlocker）。
// Building と作りは同じだが分けている: 建物は将来、中に入れるよう壁ごとの箱へ差し替える予定で、置物はその変更を受けないため。
class StaticProp : public IRayBlocker{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。isReady() 済みであること（箱の大きさを頂点から求めるため）。
	//       この置物より長く生きていること（MeshCache が持つ）。
	// position / angleY: 置き場所（地面の上）と向き（度）。
	StaticProp( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY );

	// 当たり判定の箱を world へ登録する（world が値をコピーして持つ）。world: 所有権は受け取らない。nullptr 不可。
	void addColliderTo( CollisionWorld* world ) const;
	void draw() const;
	// 弾の当たり判定（見た目の三角形で判定する）。
	virtual bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;

private:
	StaticMeshInstance mMesh;
	BoxCollider mCollider;
};

#endif
