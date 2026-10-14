#include "GameSrc/Weapon/BulletHoleList.h"
#include "GameSrc/Core/GameTime.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector2.h"
#include "GameLib/Math/Functions.h"

#include <algorithm>
#include <cmath>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const char* kTextureFile = "Resource/bullet_hole.tga"; // tools/bullet_hole_gen.py で作る

	// 同時に残す数（2026-10-14 ユーザー指定）。超えたら一番古い物から消す。
	const int kCapacity = 16;
	// 残す時間と、薄くし始めるまでの時間（ティック。2026-10-15 ユーザー指定）。5 秒を過ぎたら少しずつ薄くし、10 秒で消す。
	const int kLifeTicks = static_cast< int >( 10.f * GameTime::kTicksPerSecond );
	const int kFadeStartTicks = static_cast< int >( 5.f * GameTime::kTicksPerSecond );
	const int kFadeTicks = kLifeTicks - kFadeStartTicks; // 薄くしていく長さ

	// 板の幅の半分（m）。穴そのもの（7.62mm）では小さくて見えないので、周りの欠けを含めた大きさにする。見た目を確かめる前の初期値。
	const float kHalfSize = 0.04f;
	// 板を面から浮かせる量（m）。面とぴったり同じ奥行きだと、壁に隠れたり出たりしてちらつくため（当たり判定の表示の線と同じ値）。
	const float kLift = 0.01f;
	// 1枚ごとに板を回す角度（度。黄金角）。何枚並んでも向きがそろいにくく、同じ絵の繰り返しが目立たない。
	const float kTurnPerHoleDegree = 137.5f;
	const float kFullTurnDegree = 360.f;

	// 板の向きを作るときの仮の「上」。法線とほぼ平行（床・天井）なら外積が 0 に近づくので、もう一方を使う（Bullet::draw と同じ）。
	const Vector3 kWorldUp( 0.f, 1.f, 0.f );
	const Vector3 kWorldRight( 1.f, 0.f, 0.f );
	const float kParallelLimit = 0.99f; // 法線と上の内積がこれを超えたら、ほぼ真上・真下とみなす

	// 板の4隅（左下・右下・右上・左上）。横・縦の向きに何倍進むかと、その隅の UV（絵の左上が 0,0）。
	const int kQuadCorners = 4;
	const float kCornerSign[ kQuadCorners ][ 2 ] = { { -1.f, -1.f }, { 1.f, -1.f }, { 1.f, 1.f }, { -1.f, 1.f } };
	const Vector2 kCornerUv[ kQuadCorners ] = { Vector2( 0.f, 1.f ), Vector2( 1.f, 1.f ), Vector2( 1.f, 0.f ), Vector2( 0.f, 0.f ) };

	// 描画の容量。
	const int kVerticesPerHole = 6; // 三角形 2 つ
	const int kBatchCapacity = 2;   // 板のまとまり 1 つ＋余裕
}

BulletHoleList::BulletHoleList() :
	mNextTurnDegree( 0.f ),
	mTexture( Graphics::Texture::create( kTextureFile ) ),
	mRenderer( Scene::PrimitiveRenderer::create( kCapacity * kVerticesPerHole, kBatchCapacity ) ) {
	mHoles.reserve( kCapacity );
}

void BulletHoleList::add( const Vector3& point, const Vector3& normal, const IRayBlocker* attachedTo ) {
	ASSERT( attachedTo && "BulletHoleList : attachedTo is null." );
	if ( static_cast< int >( mHoles.size() ) >= kCapacity ) {
		mHoles.erase( mHoles.begin() ); // 一番古い物。数が少ないので、詰め直しの手間は気にしない
	}
	// 面の上で直交する2本の向きを作る。
	const Vector3& helper = ( Math::abs( normal.dot( kWorldUp ) ) > kParallelLimit ) ? kWorldRight : kWorldUp;
	Vector3 tangent;
	tangent.setCross( helper, normal );
	tangent.normalize();
	Vector3 bitangent;
	bitangent.setCross( normal, tangent );

	// 法線まわりに回してから、板の大きさにする。
	Hole hole;
	hole.right.setMul( tangent, Math::cos( mNextTurnDegree ) * kHalfSize );
	hole.right.madd( bitangent, Math::sin( mNextTurnDegree ) * kHalfSize );
	hole.up.setCross( normal, hole.right ); // normal は長さ1で right と直交するので、長さは right と同じ
	hole.center.setMadd( point, normal, kLift );
	hole.attachedTo = attachedTo;
	hole.age = 0;
	mHoles.push_back( hole );
	mNextTurnDegree = std::fmod( mNextTurnDegree + kTurnPerHoleDegree, kFullTurnDegree );
}

void BulletHoleList::update() {
	for ( Hole& hole : mHoles ) {
		++hole.age;
	}
	mHoles.erase( std::remove_if( mHoles.begin(), mHoles.end(),
		[]( const Hole& hole ) { return hole.age >= kLifeTicks; } ), mHoles.end() );
}

void BulletHoleList::removeAttachedTo( const IRayBlocker* attachedTo ) {
	mHoles.erase( std::remove_if( mHoles.begin(), mHoles.end(),
		[ attachedTo ]( const Hole& hole ) { return hole.attachedTo == attachedTo; } ), mHoles.end() );
}

void BulletHoleList::draw( const Matrix44& projectionView ) {
	// isReady を呼ぶと読み込みが進む（毎フレーム呼ぶ）。読めなかった絵では描かない。
	if ( !mTexture.isReady() || mTexture.isError() || mHoles.empty() ) {
		return;
	}
	mRenderer.setTransform( projectionView );
	mRenderer.setTexture( mTexture );
	mRenderer.enableDepthTest( true );                // 壁の向こうの跡は隠す
	mRenderer.enableDepthWrite( false );              // 跡どうし・後から描く物を隠さない
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR ); // 絵のアルファで、下の面と混ぜる
	mRenderer.setCullMode( Graphics::CULL_NONE );     // 板の裏表を気にしない（裏側は面そのものが隠す）
	for ( const Hole& hole : mHoles ) {
		addQuad( hole );
	}
	mRenderer.draw();
}

void BulletHoleList::addQuad( const Hole& hole ) {
	// kFadeStartTicks を過ぎたら、消えるまでに不透明度を 1 → 0 へ下げていく（頂点の色が絵のアルファに掛かる）。
	float opacity = ( hole.age > kFadeStartTicks ) ? static_cast< float >( kLifeTicks - hole.age ) / kFadeTicks : 1.f;
	unsigned color = VertexColor::pack( 1.f, 1.f, 1.f, opacity );

	Vector3 corner[ kQuadCorners ];
	for ( int i = 0; i < kQuadCorners; ++i ) {
		corner[ i ] = hole.center;
		corner[ i ].madd( hole.right, kCornerSign[ i ][ 0 ] );
		corner[ i ].madd( hole.up, kCornerSign[ i ][ 1 ] );
	}
	mRenderer.addTriangle( corner[ 0 ], corner[ 1 ], corner[ 2 ], kCornerUv[ 0 ], kCornerUv[ 1 ], kCornerUv[ 2 ], color, color, color );
	mRenderer.addTriangle( corner[ 0 ], corner[ 2 ], corner[ 3 ], kCornerUv[ 0 ], kCornerUv[ 2 ], kCornerUv[ 3 ], color, color, color );
}
