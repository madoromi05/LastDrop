#ifndef INCLUDED_STATIC_MODEL_COMPONENT_H
#define INCLUDED_STATIC_MODEL_COMPONENT_H

#include "GameSrc/World/Component.h"
#include "GameSrc/World/IWorldMember.h"
#include "GameSrc/Hit/IRayBlocker.h"
#include "GameSrc/Render/StaticMesh/StaticMeshInstance.h"
#include "GameSrc/Render/StaticMesh/StaticMeshLods.h"
#include "GameSrc/Render/Culling/LodSelector.h"
#include "GameSrc/Render/Culling/DrawCulling.h"

class StaticMeshData;

// 動かない置物の見た目 1 つ（車・ガソリン缶・落ちた弾。前の StaticProp の見た目の側）
// 置き場所と向きは、同じ物の TransformComponent から awake で写す（置いた後は動かない。箱を求める部品が start で読む）
// 弾・視線は見た目の三角形そのもので止める（IRayBlocker）。命中の名簿（HitWorld）へは自分で載り、物が消えるときに外れる（IWorldMember）
// 移動の当たり判定の箱は BoxColliderComponent（この見た目の頂点から求める）
class StaticModelComponent : public Component, public IRayBlocker, public IWorldMember{
public:
	// data: 形と見た目。所有権は受け取らない。nullptr 不可。isReady() 済みであること。この部品より長く生きていること（MeshCache が持つ）
	// material: 表面の材質（着弾音に使う）。cullTarget: 何として間引くか（車は Large、缶は Small）
	StaticModelComponent( const StaticMeshData* data, GroundMaterial material, DrawCulling::Target cullTarget );
	// 細かさの段（LOD）を持つ置物（車・缶）。shapes.full() が上の data にあたる（当たり判定・箱は元の形で作る）
	StaticModelComponent( const StaticMeshLods& shapes, GroundMaterial material, DrawCulling::Target cullTarget );

	// culling（映る範囲・描く距離・小ささ。DrawCulling）で描くと決まった時だけ描く
	void draw( const DrawCulling& culling ) const;
	const StaticMeshInstance& mesh() const;
	// 描く形だけを shapes に替える（街灯のガラスが光る形と消えた形。F6 で今日の条件を切り替えたとき）
	// shapes は今の形と同じ頂点の物（材質だけ違う）であること。弾・視線の当たりと箱は元の形のまま
	void setShapes( const StaticMeshLods& shapes );

	// Component（TransformComponent が要る）
	void awake() override;
	// IRayBlocker
	bool raycast( const GameLib::Math::Vector3& origin, const GameLib::Math::Vector3& delta,
		float* time, GameLib::Math::Vector3* normal ) const override;
	GroundMaterial surfaceMaterial( const GameLib::Math::Vector3& point ) const override; // 作るときに受け取った材質（どこに当たっても同じ）
	// IWorldMember（命中の名簿にだけ載る）
	void addTo( CollisionWorld* collisionWorld, HitWorld* hitWorld ) override;
	void removeFrom( CollisionWorld* collisionWorld, HitWorld* hitWorld ) override;

private:
	StaticMeshInstance mMesh;
	StaticMeshLods mShapes;
	// 今描く細かさの段。描く側だけが使う状態なので、const の draw から選び直せるように mutable（Building と同じ）
	mutable LodSelector mLod;
	GroundMaterial mMaterial;
	DrawCulling::Target mCullTarget;
};

#endif
