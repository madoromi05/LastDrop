#include "GameSrc/DebugMode/ZombiePathDrawer.h"
#include "GameSrc/Zombie/Zombie.h"
#include "GameSrc/Map/StreetGraph.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const unsigned kRouteColor = VertexColor::pack( 0.3f, 0.9f, 1.f, 1.f ); // 白い当たり判定・黄色い落ちている物と見分けられる水色
	const unsigned kSeparationColor = VertexColor::pack( 1.f, 0.55f, 0.15f, 1.f ); // 通り道の水色と見分けられる橙色
	// 線を地面から浮かせる高さ（m）。車道の面（0）・歩道（0.15）に埋もれないように。
	const float kLineLift = 0.3f;
	// 障害物の角の十字の腕の長さ（m）。
	const float kCornerMarkSize = 0.2f;
	// 分離の向きを描く長さ（m。向きの長さ 1 のとき）。
	const float kSeparationLineLength = 1.f;
	const int kVerticesPerLine = 2;
	// 一度に描ける線の数。ゾンビの上限（30 体）× 通り道の点（交差点 16 個＋角＋よける点＋行き先）と分離の線、角の十字に余裕を持たせた数。
	const int kLineCapacity = 1024;
	const int kBatchCapacity = 2; // 線のまとまり 1 つ＋余裕

	Vector3 lifted( const Vector3& ground ) {
		return Vector3( ground.x, ground.y + kLineLift, ground.z );
	}
}

void ZombiePathDrawer::draw( const ActorList< Zombie >& zombies, const StreetGraph& streets, const Matrix44& projectionView ) {
	if ( !mRenderer ) {
		mRenderer = Scene::PrimitiveRenderer::create( kLineCapacity * kVerticesPerLine, kBatchCapacity );
	}
	mRenderer.setTransform( projectionView );
	mRenderer.enableDepthTest( false ); // 建物の向こうへ回り込む道も見せる
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	mRenderer.setCullMode( Graphics::CULL_NONE );
	int lineNumber = 0;
	// 道の図に足した障害物の角（地面に置いた十字）。
	for ( const Vector3& corner : streets.obstacleCorners() ) {
		if ( lineNumber + 2 > kLineCapacity ) {
			break;
		}
		Vector3 center = lifted( corner );
		mRenderer.addLine( Vector3( center.x - kCornerMarkSize, center.y, center.z ), Vector3( center.x + kCornerMarkSize, center.y, center.z ), kRouteColor, kRouteColor );
		mRenderer.addLine( Vector3( center.x, center.y, center.z - kCornerMarkSize ), Vector3( center.x, center.y, center.z + kCornerMarkSize ), kRouteColor, kRouteColor );
		lineNumber += 2;
	}
	for ( const auto& zombie : zombies ) {
		if ( zombie->state() != Zombie::State::Alive && !zombie->isCrawling() ) {
			continue;
		}
		Vector3 from = lifted( zombie->bodyCenter() ); // 体を引きずっている体は胸から
		// 仲間を避けて混ぜた向き（混ぜていなければ描かない）。
		const Vector3& separation = zombie->separation();
		if ( ( separation.x != 0.f || separation.z != 0.f ) && lineNumber < kLineCapacity ) {
			Vector3 to;
			to.setMadd( from, separation, kSeparationLineLength );
			mRenderer.addLine( from, to, kSeparationColor, kSeparationColor );
			++lineNumber;
		}
		zombie->path().remainingRoute( &mRoute );
		for ( const Vector3& point : mRoute ) {
			if ( lineNumber >= kLineCapacity ) {
				break; // 容量を超えると止まるので、描かずに終える
			}
			Vector3 to = lifted( point );
			mRenderer.addLine( from, to, kRouteColor, kRouteColor );
			from = to;
			++lineNumber;
		}
	}
	mRenderer.draw();
}
