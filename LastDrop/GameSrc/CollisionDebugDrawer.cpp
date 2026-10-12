#include "CollisionDebugDrawer.h"
#include "CollisionWorld.h"
#include "WallMesh.h"
#include "ICollisionBody.h"
#include "HitWorld.h"
#include "IHittable.h"
#include "VertexColor.h"
#include "PlacementMatrix.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	// ── 移動の当たり判定（柱）──
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
	// キャラ 1 体で 柱 24 ＋ 部位の箱 12 個 288 頂点。キャラ 40 体＋置物 20 個ぶんに余裕を持たせた。足りなくなったら増やす。
	const int kVertexCapacity = 16000;
	const int kBatchCapacity = 2; // 線のまとまり 1 つ＋余裕

	// ── 建物の面のかけら ──
	// 線を壁の表面から浮かせる量（m）。表面とぴったり同じ奥行きだと、壁に隠れたり出たりしてちらつくため。
	const float kWallLineLift = 0.01f;
	// 名簿にある建物ぜんぶの、かけらの輪郭に要る頂点の数（かけら1つは、隅の数だけ線がある。建物1軒で約 750 個・6000 頂点）。
	int wallVertexCost( const CollisionWorld& collisionWorld ) {
		int cost = 0;
		for ( const WallMesh& walls : collisionWorld.wallMeshes() ) {
			for ( const WallPiece& piece : walls.pieces() ) {
				cost += piece.cornerNumber * kVerticesPerLine;
			}
		}
		return cost;
	}

	// ── 線分が当たった所（setRayHit）──
	const unsigned kRayHitColor = VertexColor::pack( 1.f, 0.2f, 0.2f, 1.f ); // 白い当たり判定の線と見分けられる赤
	const float kRayHitCrossHalf = 0.05f;  // 十字の腕の長さ（m）
	const float kRayHitNormalLength = 0.3f; // 法線の線の長さ（m）
	const int kRayHitAxisNumber = 3;       // 十字は X・Y・Z の3本
	const int kRayHitVertexCost = ( kRayHitAxisNumber + 1 ) * kVerticesPerLine; // 十字3本＋法線1本

	// 長方形（置物・キャラ）の4隅を、回りの順に頂点の輪にする（地面の高さ）。置き方は見た目と同じ行列（PlacementMatrix）。
	void rectRing( const BoxCollider& box, std::vector< Vector3 >* ring ) {
		const float kCornerSignX[ kRectCorners ] = { -1.f, 1.f, 1.f, -1.f };
		const float kCornerSignZ[ kRectCorners ] = { -1.f, -1.f, 1.f, 1.f };
		Matrix34 world = PlacementMatrix::make( Vector3( box.center.x, 0.f, box.center.z ), box.angleY );
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

	// ── 手前の物に隠れない柱（drawSeeThrough）──
	const unsigned kSeeThroughColor = VertexColor::pack( 1.f, 0.85f, 0.2f, 1.f ); // 白い当たり判定の線と見分けられる黄（燃料の色）
	// 一度に描ける箱の数。落ちている物（ガソリンの候補は 10 か所）に余裕を持たせた数。足りなくなったら増やす。
	const int kSeeThroughBoxCapacity = 64;
	const int kSeeThroughBatchCapacity = 2; // 線のまとまり 1 つ＋余裕

	Vector3 raised( const Vector3& ground, float height ) {
		return Vector3( ground.x, height, ground.z );
	}
}

CollisionDebugDrawer::CollisionDebugDrawer() :
	mWallVertexCapacity( 0 ),
	mHasRayHit( false ) {
}

void CollisionDebugDrawer::reserveWallVertices( const CollisionWorld& collisionWorld ) {
	int needed = wallVertexCost( collisionWorld );
	if ( mRenderer && needed <= mWallVertexCapacity ) {
		return;
	}
	// 建物は置いた後に増えないので、作り直すのはふつう最初の1回だけ。
	mWallVertexCapacity = needed;
	mRenderer = Scene::PrimitiveRenderer::create( kVertexCapacity + mWallVertexCapacity, kBatchCapacity );
}

void CollisionDebugDrawer::draw( const CollisionWorld& collisionWorld, const HitWorld& hitWorld, const Matrix44& projectionView,
const ICollisionBody* hiddenBody, const IHittable* hiddenTarget ) {
	reserveWallVertices( collisionWorld );
	collectShapes( collisionWorld, hitWorld, hiddenBody, hiddenTarget );

	mRenderer.setTransform( projectionView );
	mRenderer.enableDepthTest( true ); // 物の向こう側の線は隠す
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR ); // 線のアルファで後ろと混ぜる（kLineOpacity）
	mRenderer.setCullMode( Graphics::CULL_NONE );
	addWallLines( collisionWorld );
	for ( const Pillar& pillar : mPillars ) {
		addPillarLines( mRenderer, pillar, kLineColor );
	}
	for ( const std::vector< Vector3 >& corners : mBoxCorners ) {
		addBoxLines( corners );
	}
	addRayHitLines();
	mRenderer.draw();
}

void CollisionDebugDrawer::collectShapes( const CollisionWorld& collisionWorld, const HitWorld& hitWorld,
const ICollisionBody* hiddenBody, const IHittable* hiddenTarget ) {
	mPillars.clear();
	mBoxCorners.clear();
	int usedVertices = kRayHitVertexCost; // 線分の当たった所（addRayHitLines）のぶんを先に取っておく
	std::vector< Vector3 > shape;
	for ( const BoxCollider& box : collisionWorld.boxes() ) {
		usedVertices += pillarVertexCost( kRectCorners );
		if ( usedVertices > kVertexCapacity ) {
			return;
		}
		rectRing( box, &shape );
		mPillars.push_back( Pillar{ shape, box.height } );
	}
	for ( const ICollisionBody* body : collisionWorld.bodies() ) {
		if ( body == hiddenBody ) {
			continue;
		}
		usedVertices += pillarVertexCost( kRectCorners );
		if ( usedVertices > kVertexCapacity ) {
			return;
		}
		BoxCollider box = body->collider();
		rectRing( box, &shape );
		mPillars.push_back( Pillar{ shape, box.height } );
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

void CollisionDebugDrawer::addWallLines( const CollisionWorld& collisionWorld ) {
	// 容量は reserveWallVertices が名簿から数えて確保済みなので、ここでは数えない。
	for ( const WallMesh& walls : collisionWorld.wallMeshes() ) {
		for ( const WallPiece& piece : walls.pieces() ) {
			// 面の表の向き（三角形の回り順から決まる。メッシュの表と同じ）へ少し浮かせる。
			// 隅が一直線に並ぶかけらは向きが決まらない（長さ 0）ので、浮かせない。
			Vector3 edge0;
			Vector3 edge1;
			edge0.setSub( piece.corner[ 1 ], piece.corner[ 0 ] );
			edge1.setSub( piece.corner[ piece.cornerNumber - 1 ], piece.corner[ 0 ] );
			Vector3 lift;
			lift.setCross( edge0, edge1 );
			if ( lift.squareLength() > 0.f ) {
				lift.normalize();
				lift *= kWallLineLift;
			}
			for ( int i = 0; i < piece.cornerNumber; ++i ) {
				Vector3 from;
				Vector3 to;
				from.setAdd( piece.corner[ i ], lift );
				to.setAdd( piece.corner[ ( i + 1 ) % piece.cornerNumber ], lift );
				mRenderer.addLine( from, to, kLineColor, kLineColor );
			}
		}
	}
}

void CollisionDebugDrawer::addPillarLines( Scene::PrimitiveRenderer& renderer, const Pillar& pillar, unsigned color ) {
	const std::vector< Vector3 >& ring = pillar.ring;
	int size = static_cast< int >( ring.size() );
	for ( int i = 0; i < size; ++i ) {
		const Vector3& bottom = ring[ i ];
		const Vector3& nextBottom = ring[ ( i + 1 ) % size ];
		renderer.addLine( bottom, nextBottom, color, color );                   // 下の輪
		renderer.addLine( raised( bottom, pillar.height ), raised( nextBottom, pillar.height ), color, color ); // 上の輪
		renderer.addLine( bottom, raised( bottom, pillar.height ), color, color );             // 縦
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

void CollisionDebugDrawer::drawSeeThrough( const std::vector< BoxCollider >& boxes, const Matrix44& projectionView ) {
	if ( !mSeeThroughRenderer ) {
		mSeeThroughRenderer = Scene::PrimitiveRenderer::create( kSeeThroughBoxCapacity * pillarVertexCost( kRectCorners ), kSeeThroughBatchCapacity );
	}
	mSeeThroughRenderer.setTransform( projectionView );
	mSeeThroughRenderer.enableDepthTest( false ); // 手前の壁や建物に隠さない（向こう側にある物の場所を見せるため）
	mSeeThroughRenderer.enableDepthWrite( false );
	mSeeThroughRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	mSeeThroughRenderer.setCullMode( Graphics::CULL_NONE );
	Pillar pillar;
	int boxNumber = 0;
	for ( const BoxCollider& box : boxes ) {
		if ( boxNumber >= kSeeThroughBoxCapacity ) {
			break; // 容量を超えると止まるので、描かずに終える
		}
		rectRing( box, &pillar.ring );
		pillar.height = box.height;
		addPillarLines( mSeeThroughRenderer, pillar, kSeeThroughColor );
		++boxNumber;
	}
	mSeeThroughRenderer.draw();
}
