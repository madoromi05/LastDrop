#ifndef INCLUDED_COLLISION_DEBUG_DRAWER_H
#define INCLUDED_COLLISION_DEBUG_DRAWER_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include <vector>

class CollisionWorld;
class HitWorld;
class ICollisionBody;
class IHittable;

// 当たり判定を目に見える形で描く（開発中の確認用）。面は塗らず、頂点を結ぶ白い線だけで描く（少し透かす）。
// ・移動の当たり判定（CollisionWorld）：真上から見た 2D の形なので、高さ kPillarHeight の柱として描く
//   （下の輪・上の輪・それを結ぶ縦の線）。建物もキャラも、Y軸まわりに回した長方形。
// ・部位の当たり判定（HitWorld）：骨に付いた箱をそのまま描く（12 辺）。
// 壁の向こうにある当たり判定も見えるよう、奥行きの判定はしない（常に手前に描く）。
//
// 描くだけで、形を変えない。ON/OFF はこのクラスを呼ぶか呼ばないか（シーンが決める）。
class CollisionDebugDrawer{
public:
	CollisionDebugDrawer();

	// 両方の名簿に登録された形をすべて描く。1フレームに1回まで（PrimitiveRenderer の約束）。
	// 他のものを描いた後に呼ぶ（奥行きの判定をしないので、後から描いたものに隠されないように）。
	// projectionView: カメラの射影×ビュー行列（FirstPersonCamera::projectionViewMatrix）。
	// hiddenBody / hiddenTarget: 描かない相手（一人称の自分。自分の体を囲む線が視界を塞ぐため）。
	//                            所有権は受け取らない。nullptr なら全部描く。
	void draw( const CollisionWorld& collisionWorld, const HitWorld& hitWorld, const GameLib::Math::Matrix44& projectionView,
		const ICollisionBody* hiddenBody, const IHittable* hiddenTarget );

private:
	// 描く形を、頂点の並び（mPillarRings・mBoxCorners）にする（前のフレームのぶんは捨てる）。hidden は並べない。
	// 描画の容量（kVertexCapacity）に収まらない形は並べない（容量を超えると止まるので、描かずに飛ばす）。
	void collectShapes( const CollisionWorld& collisionWorld, const HitWorld& hitWorld,
		const ICollisionBody* hiddenBody, const IHittable* hiddenTarget );
	// 輪1つを柱の線にして積む。
	void addPillarLines( const std::vector< GameLib::Math::Vector3 >& ring );
	// 箱1つ（8 隅）の 12 辺を積む。
	void addBoxLines( const std::vector< GameLib::Math::Vector3 >& corners );

	GameLib::Scene::PrimitiveRenderer mRenderer;
	// 描く形の頂点。毎フレーム作り直す。
	std::vector< std::vector< GameLib::Math::Vector3 > > mPillarRings; // 柱の下の輪（地面の高さ）
	std::vector< std::vector< GameLib::Math::Vector3 > > mBoxCorners;  // 部位の箱の 8 隅（ワールド座標）
};

#endif
