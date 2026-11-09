#ifndef INCLUDED_COLLISION_DEBUG_DRAWER_H
#define INCLUDED_COLLISION_DEBUG_DRAWER_H

#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix44.h"
#include "GameLib/Scene/PrimitiveRenderer.h"
#include "GameSrc/Collision/BoxCollider.h"
#include <vector>

class CollisionWorld;
class HitWorld;
class ICollisionBody;
class IHittable;

// 当たり判定を目に見える形で描く（開発中の確認用）。面は塗らず、頂点を結ぶ白い線だけで描く（少し透かす）。
// ・移動の当たり判定（CollisionWorld）：真上から見た 2D の形なので、BoxCollider::height の高さの柱として描く（見た目の背丈と比べられる）
//   （下の輪・上の輪・それを結ぶ縦の線）。置物もキャラも、Y軸まわりに回した長方形。
//   建物だけは、メッシュの形に沿った面のかけら（WallMesh）を、切り取った形のまま壁の表面に描く（体の高さの範囲の帯になる）。
// ・部位の当たり判定（HitWorld）：骨に付いた箱をそのまま描く（12 辺）。
// 奥行きの判定をして、手前にある物（壁・車・体など）に隠れる所は描かない（物を透かして見えると、どの物の判定か分かりにくいため）。
// キャラ（ゾンビなど）の移動の柱も同じ（車や壁の向こうのキャラの柱が透けて見えないように）。
// 部位の箱は体の中にあるので、体の表面からはみ出した所だけが見える。
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
	// boxes（移動の当たり判定の箱）を、手前の物に隠れない黄色い線の柱で描く。壁や建物の向こうにある物の場所を探すため
	// （draw の線は手前の物に隠れるので、遠くの小さい物は見つけられない）。
	// draw とは別の描画なので、draw を呼ばないフレームでも使える。1フレームに1回まで。呼ぶ場所の決まりは draw と同じ。
	// 容量（kSeeThroughBoxCapacity 個）を超えた分は描かない。
	void drawSeeThrough( const std::vector< BoxCollider >& boxes, const GameLib::Math::Matrix44& projectionView );

private:
	// 描く形を、頂点の並び（mPillars・mBoxCorners）にする（前のフレームのぶんは捨てる）。hidden は並べない。
	// 描画の容量（kVertexCapacity）に収まらない形は並べない（容量を超えると止まるので、描かずに飛ばす）。
	void collectShapes( const CollisionWorld& collisionWorld, const HitWorld& hitWorld,
		const ICollisionBody* hiddenBody, const IHittable* hiddenTarget );
	// 移動の当たり判定1つぶんの柱（真上から見た長方形を、高さぶん立ち上げたもの）。
	struct Pillar{
		std::vector< GameLib::Math::Vector3 > ring; // 下の輪（地面の高さ）
		float height;                               // 上の輪の高さ（m）
	};

	// 建物の線に要る頂点の数を名簿から数え、今の容量で足りなければ mRenderer を作り直す（draw の最初に呼ぶ）。
	// 容量を決まった数にしないのは、建物の数は町の作りで大きく変わり（1軒で約 6000 頂点）、
	// 足りないぶんを黙って描かないと「表示が出ない」ように見えるため（町を 96 軒にしたときに起きた）。
	void reserveWallVertices( const CollisionWorld& collisionWorld );
	// 建物の面のかけらの輪郭を積む（毎フレーム名簿から直接読む。数が多いので、頂点の並びを作り直さない）。
	void addWallLines( const CollisionWorld& collisionWorld );
	// 柱1本の線を renderer へ積む。color: 線の色（VertexColor::pack の並び）。
	static void addPillarLines( GameLib::Scene::PrimitiveRenderer& renderer, const Pillar& pillar, unsigned color );
	// 箱1つ（8 隅）の 12 辺を積む。
	void addBoxLines( const std::vector< GameLib::Math::Vector3 >& corners );
	// 当たった点の十字と法線の線を積む（setRayHit されていれば）。
	void addRayHitLines();

	GameLib::Scene::PrimitiveRenderer mRenderer; // すべての線（手前の物に隠れる）。最初の draw で作る（それまでは空）
	int mWallVertexCapacity;                     // mRenderer が建物の線のために持っている頂点の数
	GameLib::Scene::PrimitiveRenderer mSeeThroughRenderer; // drawSeeThrough の線（手前の物に隠れない）。最初の drawSeeThrough で作る
	// 描く形の頂点。毎フレーム作り直す。
	std::vector< Pillar > mPillars;                                    // 移動の当たり判定の柱
	std::vector< std::vector< GameLib::Math::Vector3 > > mBoxCorners;  // 部位の箱の 8 隅（ワールド座標）
	GameLib::Math::Vector3 mRayHitPoint;  // setRayHit で受け取った点と法線
	GameLib::Math::Vector3 mRayHitNormal;
	bool mHasRayHit;                      // 次の draw で描くか
};

#endif
