#include "GameSrc/DebugMode/ZombiePathDrawer.h"
#include "GameSrc/Zombie/Zombie.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const unsigned kRouteColor = VertexColor::pack( 0.3f, 0.9f, 1.f, 1.f ); // 白い当たり判定・黄色い落ちている物と見分けられる水色
	// 線を地面から浮かせる高さ（m）。車道の面（0）・歩道（0.15）に埋もれないように。
	const float kLineLift = 0.3f;
	const int kVerticesPerLine = 2;
	// 一度に描ける線の数。ゾンビの上限（30 体）× 通り道の点（交差点 16 個＋行き先）に余裕を持たせた数。
	const int kLineCapacity = 1024;
	const int kBatchCapacity = 2; // 線のまとまり 1 つ＋余裕

	Vector3 lifted( const Vector3& ground ) {
		return Vector3( ground.x, ground.y + kLineLift, ground.z );
	}
}

void ZombiePathDrawer::draw( const ActorList< Zombie >& zombies, const Matrix44& projectionView ) {
	if ( !mRenderer ) {
		mRenderer = Scene::PrimitiveRenderer::create( kLineCapacity * kVerticesPerLine, kBatchCapacity );
	}
	mRenderer.setTransform( projectionView );
	mRenderer.enableDepthTest( false ); // 建物の向こうへ回り込む道も見せる
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	mRenderer.setCullMode( Graphics::CULL_NONE );
	int lineNumber = 0;
	for ( const auto& zombie : zombies ) {
		if ( zombie->state() != Zombie::State::Alive ) {
			continue;
		}
		zombie->path().remainingRoute( &mRoute );
		Vector3 from = lifted( zombie->position() );
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
