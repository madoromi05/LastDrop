#include "GameSrc/DebugMode/TownEditPreview.h"

#ifdef ZF_EDITOR

#include "GameSrc/DebugMode/TownEditPanel.h"
#include "GameSrc/DebugMode/TownEditFrame.h"
#include "GameSrc/Player/Camera/FirstPersonCamera.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Vector3.h"
#include "GameLib/Math/Matrix34.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const unsigned kOriginalColor = VertexColor::pack( 0.6f, 0.6f, 0.6f, 1.f );	// 元の位置
	const unsigned kCurrentColor = VertexColor::pack( 1.f, 0.85f, 0.2f, 1.f );	// 今の位置（Town タブの、手直しした物の字と同じ黄色）
	// -1〜1 の立方体の隅。番号の 1・2・4 の位が、それぞれ X・Y・Z の -1/+1 にあたる（隅 i と i|位 を結ぶと辺になる）
	const int kCubeCorners = 8;
	const int kCubeAxisBits[] = { 1, 2, 4 };
	const int kCubeEdges = 12;
	const float kCubeHalf = 1.f;
	const int kVerticesPerLine = 2;
	const int kFrameNumber = 2;			// 元の位置と今の位置
	const int kVertexCapacity = kFrameNumber * kCubeEdges * kVerticesPerLine;
	const int kBatchCapacity = 2;		// 線のまとまり 1 つ＋余裕

	void addFrameLines( Scene::PrimitiveRenderer* renderer, const Matrix34& box, unsigned color ) {	// 枠の箱の 12 辺を積む。1 つの位だけ違う隅どうしを結ぶ
		Vector3 corners[ kCubeCorners ];
		for ( int i = 0; i < kCubeCorners; ++i ) {
			const Vector3 local(
				( i & kCubeAxisBits[ 0 ] ) ? kCubeHalf : -kCubeHalf,
				( i & kCubeAxisBits[ 1 ] ) ? kCubeHalf : -kCubeHalf,
				( i & kCubeAxisBits[ 2 ] ) ? kCubeHalf : -kCubeHalf );
			box.mul( &corners[ i ], local );
		}
		for ( int i = 0; i < kCubeCorners; ++i ) {
			for ( int bit : kCubeAxisBits ) {
				if ( !( i & bit ) ) {
					renderer->addLine( corners[ i ], corners[ i | bit ], color, color );
				}
			}
		}
	}
}

void TownEditPreview::draw( const FirstPersonCamera& camera ) {
	const TownEditPanel::Highlight* highlight = TownEditPanel::highlight();
	if ( !highlight ) {
		return;
	}
	if ( !mRenderer ) {
		mRenderer = Scene::PrimitiveRenderer::create( kVertexCapacity, kBatchCapacity );
	}
	mRenderer.setTransform( camera.projectionViewMatrix() );
	mRenderer.enableDepthTest( false );	// 建物の向こうの物も見せる
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	mRenderer.setCullMode( Graphics::CULL_NONE );
	if ( highlight->hasOriginal ) {
		addFrameLines( &mRenderer, TownEditFrame::boxMatrix( highlight->kind, highlight->original ), kOriginalColor );
	}
	if ( highlight->hasCurrent ) {
		addFrameLines( &mRenderer, TownEditFrame::boxMatrix( highlight->kind, highlight->current ), kCurrentColor );	// 動かしていなければ元の枠に重なるので、後から描いて黄色を見せる
	}
	mRenderer.draw();
}

#endif // ZF_EDITOR
