#include "CollisionDebugDrawer.h"
#include "CollisionWorld.h"
#include "ICollisionBody.h"
#include "HitWorld.h"
#include "IHittable.h"
#include "VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 移動の当たり判定（柱）──
	const float kPillarHeight = 2.f;  // 柱の高さ（m）。人の背丈より少し高くし、ゾンビや自分の体に埋もれないようにする
	const int kRectCorners = 4;       // 真上から見た長方形の隅の数

	// ── 部位の当たり判定（箱）──
	// 隅の番号の 1・2・4 の位が、それぞれ X・Y・Z の -1/+1 にあたる（隅 i と i|位 を結ぶと辺になる）。
	const int kCubeCorners = 8;
	const int kCubeAxisBits[] = { 1, 2, 4 };
	const int kCubeEdges = 12;
	const float kCubeHalf = 1.f; // HitboxSet::boxMatrix が置く立方体の幅の半分（-1〜1）

	// 線の色（VertexColor と同じ詰め方。メッシュの頂点カラーと同じ並び）。白を少し透かして、後ろの体や壁も見えるようにする。
	const float kLineOpacity = 0.5f; // 不透明度（1 で不透明、0 で見えない）
	const unsigned kLineColor = VertexColor::pack( 1.f, 1.f, 1.f, kLineOpacity );

	// ── 容量 ──
	const int kVerticesPerLine = 2;
	// 柱1本（輪の頂点 n 個）：「下の輪 n 本＋上の輪 n 本＋縦 n 本」の線。
	const int kPillarLinesPerCorner = 3;
	int pillarVertexCost( int ringSize ) {
		return ringSize * kPillarLinesPerCorner * kVerticesPerLine;
	}
	// 部位の箱1つ：線 12 本。
	const int kBoxVertexCost = kCubeEdges * kVerticesPerLine;
	// キャラ 1 体で 柱 24 ＋ 部位の箱 12 個 288 頂点。キャラ 40 体＋建物 20 軒ぶんに余裕を持たせた。足りなくなったら増やす。
	const int kVertexCapacity = 16000;
	const int kBatchCapacity = 2; // 線のまとまり 1 つ＋余裕

	// ── 線分が当たった所（setRayHit）──
	const unsigned kRayHitColor = VertexColor::pack( 1.f, 0.2f, 0.2f, 1.f ); // 白い当たり判定の線と見分けられる赤
	const float kRayHitCrossHalf = 0.05f;  // 十字の腕の長さ（m）
	const float kRayHitNormalLength = 0.3f; // 法線の線の長さ（m）
	const int kRayHitAxisNumber = 3;       // 十字は X・Y・Z の3本
	const int kRayHitVertexCost = ( kRayHitAxisNumber + 1 ) * kVerticesPerLine; // 十字3本＋法線1本

	// 長方形（建物・キャラ）の4隅を、回りの順に頂点の輪にする（地面の高さ）。置き方は Building と同じく Matrix34 の 移動*回転。
	void rectRing( const BoxCollider& box, std::vector< Vector3 >* ring ) {
		const float kCornerSignX[ kRectCorners ] = { -1.f, 1.f, 1.f, -1.f };
		const float kCornerSignZ[ kRectCorners ] = { -1.f, -1.f, 1.f, 1.f };
		Matrix34 world;
		world.setTranslation( Vector3( box.center.x, 0.f, box.center.z ) );
		world.rotateY( box.angleY );
		ring->clear();
		for ( int i = 0; i < kRectCorners; ++i ) {
			Vector3 corner;
			world.mul( &corner, Vector3( kCornerSignX[ i ] * box.halfWidthX, 0.f, kCornerSignZ[ i ] * box.halfWidthZ ) );
			ring->push_back( corner );
		}
	}

	// -1〜1 の立方体を boxMatrix で置いた箱の 8 隅（番号の決まりは kCubeAxisBits）。
	void cubeCorners( const Matrix34& boxMatrix, std::vector< Vector3 >* corners ) {
		corners->clear();
		for ( int i = 0; i < kCubeCorners; ++i ) {
			Vector3 local(
				( i & kCubeAxisBits[ 0 ] ) ? kCubeHalf : -kCubeHalf,
				( i & kCubeAxisBits[ 1 ] ) ? kCubeHalf : -kCubeHalf,
				( i & kCubeAxisBits[ 2 ] ) ? kCubeHalf : -kCubeHalf );
			Vector3 corner;
			boxMatrix.mul( &corner, local );
			corners->push_back( corner );
		}
	}

	Vector3 raised( const Vector3& ground ) {
		return Vector3( ground.x, ground.y + kPillarHeight, ground.z );
	}
}

CollisionDebugDrawer::CollisionDebugDrawer() :
	mRenderer( Scene::PrimitiveRenderer::create( kVertexCapacity, kBatchCapacity ) ),
	mHasRayHit( false ) {
}

void CollisionDebugDrawer::draw( const CollisionWorld& collisionWorld, const HitWorld& hitWorld, const Matrix44& projectionView,
const ICollisionBody* hiddenBody, const IHittable* hiddenTarget ) {
	collectShapes( collisionWorld, hitWorld, hiddenBody, hiddenTarget );

	mRenderer.setTransform( projectionView );
	mRenderer.enableDepthTest( false ); // 壁の向こうも見せる
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR ); // 線のアルファで後ろと混ぜる（kLineOpacity）
	mRenderer.setCullMode( Graphics::CULL_NONE );
	for ( const std::vector< Vector3 >& ring : mPillarRings ) {
		addPillarLines( ring );
	}
	for ( const std::vector< Vector3 >& corners : mBoxCorners ) {
		addBoxLines( corners );
	}
	addRayHitLines();
	mRenderer.draw();
}

void CollisionDebugDrawer::collectShapes( const CollisionWorld& collisionWorld, const HitWorld& hitWorld,
const ICollisionBody* hiddenBody, const IHittable* hiddenTarget ) {
	mPillarRings.clear();
	mBoxCorners.clear();
	int usedVertices = kRayHitVertexCost; // 線分の当たった所（addRayHitLines）のぶんを先に取っておく
	std::vector< Vector3 > shape;
	for ( const BoxCollider& box : collisionWorld.boxes() ) {
		usedVertices += pillarVertexCost( kRectCorners );
		if ( usedVertices > kVertexCapacity ) {
			return;
		}
		rectRing( box, &shape );
		mPillarRings.push_back( shape );
	}
	for ( const ICollisionBody* body : collisionWorld.bodies() ) {
		if ( body == hiddenBody ) {
			continue;
		}
		usedVertices += pillarVertexCost( kRectCorners );
		if ( usedVertices > kVertexCapacity ) {
			return;
		}
		rectRing( body->collider(), &shape );
		mPillarRings.push_back( shape );
	}
	for ( const IHittable* target : hitWorld.targets() ) {
		if ( target == hiddenTarget ) {
			continue;
		}
		const HitboxSet& hitboxes = target->hitboxes();
		for ( int i = 0; i < hitboxes.boxNumber(); ++i ) {
			Matrix34 boxMatrix;
			if ( !hitboxes.boxMatrix( i, &boxMatrix ) ) {
				continue;
			}
			usedVertices += kBoxVertexCost;
			if ( usedVertices > kVertexCapacity ) {
				return;
			}
			cubeCorners( boxMatrix, &shape );
			mBoxCorners.push_back( shape );
		}
	}
}

void CollisionDebugDrawer::addPillarLines( const std::vector< Vector3 >& ring ) {
	int size = static_cast< int >( ring.size() );
	for ( int i = 0; i < size; ++i ) {
		const Vector3& bottom = ring[ i ];
		const Vector3& nextBottom = ring[ ( i + 1 ) % size ];
		mRenderer.addLine( bottom, nextBottom, kLineColor, kLineColor );                   // 下の輪
		mRenderer.addLine( raised( bottom ), raised( nextBottom ), kLineColor, kLineColor ); // 上の輪
		mRenderer.addLine( bottom, raised( bottom ), kLineColor, kLineColor );             // 縦
	}
}

void CollisionDebugDrawer::addBoxLines( const std::vector< Vector3 >& corners ) {
	// 1 つの位だけ違う隅どうしを結ぶと、立方体の 12 辺になる。
	for ( int i = 0; i < kCubeCorners; ++i ) {
		for ( int bit : kCubeAxisBits ) {
			if ( i & bit ) {
				continue;
			}
			mRenderer.addLine( corners[ i ], corners[ i | bit ], kLineColor, kLineColor );
		}
	}
}

void CollisionDebugDrawer::setRayHit( const Vector3& point, const Vector3& normal ) {
	mRayHitPoint = point;
	mRayHitNormal = normal;
	mHasRayHit = true;
}

void CollisionDebugDrawer::addRayHitLines() {
	if ( !mHasRayHit ) {
		return;
	}
	mHasRayHit = false; // 1回の draw ぶんだけ描く（当たらなくなったら消えるように）
	for ( int axis = 0; axis < kRayHitAxisNumber; ++axis ) {
		Vector3 arm( 0.f, 0.f, 0.f );
		arm[ axis ] = kRayHitCrossHalf;
		Vector3 from;
		Vector3 to;
		from.setSub( mRayHitPoint, arm );
		to.setAdd( mRayHitPoint, arm );
		mRenderer.addLine( from, to, kRayHitColor, kRayHitColor );
	}
	Vector3 tip;
	tip.setMadd( mRayHitPoint, mRayHitNormal, kRayHitNormalLength );
	mRenderer.addLine( mRayHitPoint, tip, kRayHitColor, kRayHitColor );
}
