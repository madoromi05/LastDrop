#include "GameSrc/DebugMode/ZombiePathDrawer.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieChaseComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const unsigned kRouteColor = VertexColor::pack( 0.3f, 0.9f, 1.f, 1.f );			// 白い当たり判定・黄色い落ちている物と見分けられる水色
	const unsigned kSeparationColor = VertexColor::pack( 1.f, 0.55f, 0.15f, 1.f );	// 通り道の水色と見分けられる橙色
	const float kLineLift = 0.3f;													// 線を地面から浮かせる高さ（m）。車道の面（0）・歩道（0.15）・床の辺の線（0.2。NavMeshDrawer）に埋もれないように
	// 分離の向きを描く長さ（m。向きの長さ 1 のとき）
	const float kSeparationLineLength = 1.f;
	const unsigned kAimColor = VertexColor::pack( 1.f, 0.35f, 1.f, 1.f );			// 先読みで向いている点への線。水色・橙色・黄色と見分けられる紫
	// 引っかかった体の頭の上の印（ZombieMoveTrace の印が出ている間）。足元からの高さ（m）と十字の腕の長さ（m）
	const unsigned kStuckColor = VertexColor::pack( 1.f, 0.15f, 0.15f, 1.f );
	const float kStuckMarkerBottom = 2.f;
	const float kStuckMarkerTop = 2.8f;
	const float kStuckCrossHalf = 0.3f;
	const int kStuckMarkerLines = 3;												// 印 1 つの線の数（縦 1 本＋十字 2 本）
	const int kVerticesPerLine = 2;
	const int kLineCapacity = 1024;													// 一度に描ける線の数。ゾンビの上限 × 通り道の点（曲がる角＋よける点＋行き先）・分離・先読み・引っかかりの印の線に余裕を持たせた数
	const int kBatchCapacity = 2;													// 線のまとまり 1 つ＋余裕

	Vector3 lifted( const Vector3& ground, float lift ) {
		return Vector3( ground.x, ground.y + lift, ground.z );
	}
}

void ZombiePathDrawer::draw( const GameObjectWorld& objects, const Matrix44& projectionView ) {
	if ( !mRenderer ) {
		mRenderer = Scene::PrimitiveRenderer::create( kLineCapacity * kVerticesPerLine, kBatchCapacity );
	}
	mRenderer.setTransform( projectionView );
	mRenderer.enableDepthTest( false ); // 建物の向こうへ回り込む道も見せる
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	mRenderer.setCullMode( Graphics::CULL_NONE );
	int lineNumber = 0;
	objects.forEach< ZombieStateComponent >( [ this, &lineNumber ]( const ZombieStateComponent& zombie ) {
		if ( zombie.state() != ZombieStateComponent::State::Alive && !zombie.isCrawling() ) {
			return;
		}
		Vector3 from = lifted( zombie.bodyCenter(), kLineLift ); // 体を引きずっている体は胸から
		const ZombieChaseComponent* chase = zombie.getComponent< ZombieChaseComponent >();
		// 仲間を避けて混ぜた向き（混ぜていなければ描かない）
		const Vector3& separation = chase->separation();
		if ( ( separation.x != 0.f || separation.z != 0.f ) && lineNumber < kLineCapacity ) {
			Vector3 to;
			to.setMadd( from, separation, kSeparationLineLength );
			mRenderer.addLine( from, to, kSeparationColor, kSeparationColor );
			++lineNumber;
		}
		const ZombieMoveTrace& trace = chase->locomotion().trace();
		// 通り道に沿って先読みで向いている点（追う・探す間だけ。うろつく間は行き先そのものなので描かない）
		if ( chase->mode() != ZombieChase::Mode::Wander && lineNumber < kLineCapacity ) {
			Vector3 to = lifted( trace.aimPoint(), kLineLift );
			mRenderer.addLine( from, to, kAimColor, kAimColor );
			++lineNumber;
		}
		if ( trace.isMarked() && lineNumber + kStuckMarkerLines <= kLineCapacity ) {
			addStuckMarker( zombie.position() );
			lineNumber += kStuckMarkerLines;
		}
		chase->path().remainingRoute( &mRoute );
		for ( const Vector3& point : mRoute ) {
			if ( lineNumber >= kLineCapacity ) {
				break; // 容量を超えると止まるので、描かずに終える
			}
			Vector3 to = lifted( point, kLineLift );
			mRenderer.addLine( from, to, kRouteColor, kRouteColor );
			from = to;
			++lineNumber;
		}
	} );
	mRenderer.draw();
}

void ZombiePathDrawer::addStuckMarker( const Vector3& feet ) {
	Vector3 bottom( feet.x, feet.y + kStuckMarkerBottom, feet.z );
	Vector3 top( feet.x, feet.y + kStuckMarkerTop, feet.z );
	mRenderer.addLine( bottom, top, kStuckColor, kStuckColor );
	// 上の端に、真上から見て十字になる 2 本（どの向きから見ても線のどちらかが見える）
	mRenderer.addLine( Vector3( top.x - kStuckCrossHalf, top.y, top.z ), Vector3( top.x + kStuckCrossHalf, top.y, top.z ), kStuckColor, kStuckColor );
	mRenderer.addLine( Vector3( top.x, top.y, top.z - kStuckCrossHalf ), Vector3( top.x, top.y, top.z + kStuckCrossHalf ), kStuckColor, kStuckColor );
}
