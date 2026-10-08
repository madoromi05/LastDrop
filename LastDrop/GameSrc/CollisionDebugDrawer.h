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
// ・移動の当たり判定（CollisionWorld）：真上から見た 2D の形なので、BoxCollider::height の高さの柱として描く（見た目の背丈と比べられる）
//   （下の輪・上の輪・それを結ぶ縦の線）。建物もキャラも、Y軸まわりに回した長方形。
// ・部位の当たり判定（HitWorld）：骨に付いた箱をそのまま描く（12 辺）。
// 奥行きの判定をして、手前にある物（壁・車・体など）に隠れる所は描かない（物を透かして見えると、どの物の判定か分かりにくいため）。
// ただしキャラ（ゾンビなど）の移動の柱だけは、前と同じく物を透かして描く（柱は体の中に立つので、隠すとほとんど見えなくなるため）。
// 線は奥行きを書き込まない（線どうし・後から描く物を隠さない）。
//
// 描くだけで、形を変えない。ON/OFF はこのクラスを呼ぶか呼ばないか（シーンが決める）。
class CollisionDebugDrawer{
public:
	CollisionDebugDrawer();

	// 両方の名簿に登録された形をすべて描く。1フレームに1回まで（PrimitiveRenderer の約束）。
	// 他のものを描いた後、同じ描画先（PostProcess の beginScene〜endScene の間）で呼ぶ（物の奥行きで線を隠すため）。
	// projectionView: カメラの射影×ビュー行列（FirstPersonCamera::projectionViewMatrix）。
	// hiddenBody / hiddenTarget: 描かない相手（一人称の自分。自分の体を囲む線が視界を塞ぐため）。
	//                            所有権は受け取らない。nullptr なら全部描く。
	void draw( const CollisionWorld& collisionWorld, const HitWorld& hitWorld, const GameLib::Math::Matrix44& projectionView,
		const ICollisionBody* hiddenBody, const IHittable* hiddenTarget );
	// 次の draw で、線分が当たった点（小さな十字）と面の法線（短い線）を赤で一緒に描く。draw の後に消える。
	// 弾の当たり判定（HitWorld::raycast）が正しい位置・向きを返しているかを目で確かめるため。
	void setRayHit( const GameLib::Math::Vector3& point, const GameLib::Math::Vector3& normal );

private:
	// 描く形を、頂点の並び（mPillars・mBoxCorners）にする（前のフレームのぶんは捨てる）。hidden は並べない。
	// 描画の容量（kVertexCapacity）に収まらない形は並べない（容量を超えると止まるので、描かずに飛ばす）。
	void collectShapes( const CollisionWorld& collisionWorld, const HitWorld& hitWorld,
		const ICollisionBody* hiddenBody, const IHittable* hiddenTarget );
	// 移動の当たり判定1つぶんの柱（真上から見た長方形を、高さぶん立ち上げたもの）。
	struct Pillar{
		std::vector< GameLib::Math::Vector3 > ring; // 下の輪（地面の高さ）
		float height;                               // 上の輪の高さ（m）
		bool seeThrough;                            // true: 物を透かして描く（キャラの柱）。false: 手前の物に隠れる
	};

	// 柱1本の線を積む。
	void addPillarLines( const Pillar& pillar );
	// 箱1つ（8 隅）の 12 辺を積む。
	void addBoxLines( const std::vector< GameLib::Math::Vector3 >& corners );
	// 当たった点の十字と法線の線を積む（setRayHit されていれば）。
	void addRayHitLines();

	GameLib::Scene::PrimitiveRenderer mRenderer;           // 手前の物に隠れる線
	GameLib::Scene::PrimitiveRenderer mSeeThroughRenderer; // 物を透かして描く線（キャラの柱）。奥行きの設定は描画ごとなので分ける
	// 描く形の頂点。毎フレーム作り直す。
	std::vector< Pillar > mPillars;                                    // 移動の当たり判定の柱
	std::vector< std::vector< GameLib::Math::Vector3 > > mBoxCorners;  // 部位の箱の 8 隅（ワールド座標）
	GameLib::Math::Vector3 mRayHitPoint;  // setRayHit で受け取った点と法線
	GameLib::Math::Vector3 mRayHitNormal;
	bool mHasRayHit;                      // 次の draw で描くか
};

#endif
