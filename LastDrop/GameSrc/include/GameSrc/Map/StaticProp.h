#ifndef INCLUDED_STATIC_PROP_H
#define INCLUDED_STATIC_PROP_H

#include "GameLib/Math/Vector3.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"
#include "GameSrc/Collision/BoxCollider.h"
#include "GameSrc/Hit/IRayBlocker.h"

class ViewFrustum;
class StaticMeshData;
class CollisionWorld;

// 動かない置物1つ（車・ガソリン缶など）。見た目（StaticMeshInstance）と、頂点を囲む当たり判定の箱1つを、同じ置き場所・向きから作って持つ。
// 弾は見た目の三角形そのもので止める（IRayBlocker）。
// 置いた後に名簿からどう外すか（外さない車・拾うと消える缶）は持ち主が決める。ここは「見た目と箱を同じ置き場所から作る」だけを受け持ち、
// 車（EscapeCar）も拾える物（Pickup）もこれを部品として持つ。
// Building と分けている: 建物の移動の当たり判定はメッシュの形に沿わせる（MeshWallBuilder）が、置物は箱1つで足りるため。
class StaticProp : public IRayBlocker{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。isReady() 済みであること（箱の大きさを頂点から求めるため）。
	//       この置物より長く生きていること（MeshCache が持つ）。
	// position / angleY: 置き場所（地面の上）と向き（度）。
	// material: 表面の材質（着弾音に使う）。何でできた物かは持ち主が決める。
	StaticProp( const StaticMeshData* data, const GameLib::Math::Vector3& position, float angleY, GroundMaterial material );

	// 当たり判定の箱を、動かない箱として world へ登録する（world が値をコピーして持つ。後から外せない）。
	// 最後まで置いておく物（車）用。途中で消える物は、持ち主が collider() を外せる形で載せる（Pickup）。
	// world: 所有権は受け取らない。nullptr 不可。
	void addColliderTo( CollisionWorld* world ) const;
	// 当たり判定の箱（頂点を真上から囲む長方形。置いた所から動かない）。
	const BoxCollider& collider() const;
	// frustum: カメラに映る範囲。映らなければ描かない。
	void draw( const ViewFrustum& frustum ) const;
	// 弾の当たり判定（見た目の三角形で判定する）。
	virtual bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const;
	// 作るときに受け取った材質（どこに当たっても同じ）。
	virtual GroundMaterial surfaceMaterial( const GameLib::Math::Vector3& point ) const;

private:
	StaticMeshInstance mMesh;
	BoxCollider mCollider;
	GroundMaterial mMaterial;
};

#endif
