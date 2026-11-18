#include "GameSrc/DebugMode/NavMeshDrawer.h"
#include "GameSrc/Map/NavMesh.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Graphics/Enum.h"

#include <vector>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const float kLift = 0.2f;												// 線を地面から浮かせる高さ（m）。歩道の面（0.15m）より少し上で、ゾンビの通り道の線（0.3m）より低い
	const unsigned kColor = VertexColor::pack( 0.3f, 0.8f, 0.4f, 0.35f );	// 数が多いので、ゾンビの通り道の水色より暗く薄い緑で目立たせない
	const int kVerticesPerLine = 2;
	const int kBatchCapacity = 2;											// 線のまとまり 1 つ＋余裕

	Vector3 lifted( const Vector3& ground ) {
		return Vector3( ground.x, ground.y + kLift, ground.z );
	}
}

NavMeshDrawer::NavMeshDrawer() :
	mLineCapacity( 0 ) {
}

void NavMeshDrawer::draw( const NavMesh& navMesh, const Matrix44& projectionView ) {
	const std::vector< Vector3 >& outline = navMesh.outline();
	int lineNumber = static_cast< int >( outline.size() ) / kVerticesPerLine;
	if ( lineNumber == 0 ) {
		return; // 床ができていない（作るのに失敗した）
	}
	// 床の辺の数は町の形で決まる。前に作った器で足りなければ作り直す
	if ( !mRenderer || mLineCapacity < lineNumber ) {
		mRenderer = Scene::PrimitiveRenderer::create( lineNumber * kVerticesPerLine, kBatchCapacity );
		mLineCapacity = lineNumber;
	}
	mRenderer.setTransform( projectionView );
	mRenderer.enableDepthTest( false ); // 建物の向こうの床も見せる
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	mRenderer.setCullMode( Graphics::CULL_NONE );
	for ( size_t i = 0; i + 1 < outline.size(); i += kVerticesPerLine ) {
		mRenderer.addLine( lifted( outline[ i ] ), lifted( outline[ i + 1 ] ), kColor, kColor );
	}
	mRenderer.draw();
}
