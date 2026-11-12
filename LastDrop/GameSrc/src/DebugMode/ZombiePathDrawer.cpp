#include "GameSrc/DebugMode/ZombiePathDrawer.h"
#include "GameSrc/Zombie/Component/ZombieStateComponent.h"
#include "GameSrc/Zombie/Component/ZombieChaseComponent.h"
#include "GameSrc/World/GameObject.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/World/GameObjectWorld.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const unsigned kRouteColor = VertexColor::pack( 0.3f, 0.9f, 1.f, 1.f );			// 白い当たり判定・黄色い落ちている物と見分けられる水色
	const unsigned kSeparationColor = VertexColor::pack( 1.f, 0.55f, 0.15f, 1.f );	// 通り道の水色と見分けられる橙色
	const float kLineLift = 0.3f;													// 線を地面から浮かせる高さ（m）。車道の面（0）・歩道（0.15）に埋もれないように
	const float kOutlineLift = 0.2f;												// 床の縁は通り道より低く浮かせる（歩道の面 0.15m より少し上）
	const unsigned kOutlineColor = VertexColor::pack( 0.3f, 0.8f, 0.4f, 0.35f );	// 床の縁の色。数が多いので、通り道の水色より暗く薄い緑で目立たせない
	// 分離の向きを描く長さ（m。向きの長さ 1 のとき）
	const float kSeparationLineLength = 1.f;
	const int kVerticesPerLine = 2;
	const int kRouteLineCapacity = 1024;											// 床の縁のほかに一度に描ける線の数。ゾンビの上限（30 体）× 通り道の点（曲がる角＋よける点＋行き先）と分離の線に余裕を持たせた数
	const int kBatchCapacity = 2;													// 線のまとまり 1 つ＋余裕

	Vector3 lifted( const Vector3& ground, float lift ) {
		return Vector3( ground.x, ground.y + lift, ground.z );
	}
}

ZombiePathDrawer::ZombiePathDrawer() :
	mLineCapacity( 0 ) {
}

void ZombiePathDrawer::draw( const GameObjectWorld& objects, const NavMesh& navMesh, const Matrix44& projectionView ) {
	const std::vector< Vector3 >& outline = navMesh.outline();
	// 床の縁の線の数は町の形で決まる。床ができる前（町の読み込み中）に作った器では足りないので、そのときは作り直す
	int lineCapacity = kRouteLineCapacity + static_cast< int >( outline.size() ) / kVerticesPerLine;
	if ( !mRenderer || mLineCapacity < lineCapacity ) {
		mRenderer = Scene::PrimitiveRenderer::create( lineCapacity * kVerticesPerLine, kBatchCapacity );
		mLineCapacity = lineCapacity;
	}
	mRenderer.setTransform( projectionView );
	mRenderer.enableDepthTest( false ); // 建物の向こうへ回り込む道も見せる
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	mRenderer.setCullMode( Graphics::CULL_NONE );
	int lineNumber = 0;
	// 歩ける床の多角形の縁（ゾンビが歩ける範囲。壁から体の半径だけ離れている）
	for ( size_t i = 0; i + 1 < outline.size(); i += kVerticesPerLine ) {
		mRenderer.addLine( lifted( outline[ i ], kOutlineLift ), lifted( outline[ i + 1 ], kOutlineLift ), kOutlineColor, kOutlineColor );
		++lineNumber;
	}
	objects.forEach< ZombieStateComponent >( [ this, &lineNumber ]( const ZombieStateComponent& zombie ) {
		if ( zombie.state() != ZombieStateComponent::State::Alive && !zombie.isCrawling() ) {
			return;
		}
		Vector3 from = lifted( zombie.bodyCenter(), kLineLift ); // 体を引きずっている体は胸から
		// 仲間を避けて混ぜた向き（混ぜていなければ描かない）
		const Vector3& separation = zombie.getComponent< ZombieChaseComponent >()->separation();
		if ( ( separation.x != 0.f || separation.z != 0.f ) && lineNumber < mLineCapacity ) {
			Vector3 to;
			to.setMadd( from, separation, kSeparationLineLength );
			mRenderer.addLine( from, to, kSeparationColor, kSeparationColor );
			++lineNumber;
		}
		zombie.getComponent< ZombieChaseComponent >()->path().remainingRoute( &mRoute );
		for ( const Vector3& point : mRoute ) {
			if ( lineNumber >= mLineCapacity ) {
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
