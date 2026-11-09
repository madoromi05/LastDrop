#include "GameSrc/DebugMode/LightDebugDrawer.h"
#include "GameSrc/Render/Lighting/LightList.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/Graphics/Enum.h"
#include "GameLib/Math/Functions.h"

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const unsigned kLitColor = VertexColor::pack( 1.f, 0.9f, 0.2f, 1.f );     // 照らしている（街灯の色に近い黄色）
	const unsigned kDroppedColor = VertexColor::pack( 0.5f, 0.5f, 0.5f, 1.f ); // 16 個に入らず消えている
	const float kGroundHeight = 0.f;   // 届く範囲の円を描く高さ（車道の面。TownPlacement の y と同じ基準）
	const float kLineLift = 0.3f;      // 線を地面から浮かせる高さ（m。ZombiePathDrawer と同じ。歩道に埋もれないように）
	const float kMarkSize = 0.3f;      // 光の位置の十字の腕の長さ（m）
	const int kCircleSegments = 24;    // 円を何本の線で描くか
	const float kFullTurnDegree = 360.f;
	const float kSpotLineLength = 3.f; // スポットの照らす向きの線の長さ（m）
	const int kMarkLines = 5;          // 十字 3 本＋縦の線 1 本＋スポットの向き 1 本
	const int kVerticesPerLine = 2;
	// 一度に描ける線の数。ライト 16 個より多め（街灯 38＋懐中電灯＋マズルフラッシュ。選ばれなかった物も描く）× 1 個ぶんの線。
	const int kMaxDrawnLights = 48;
	const int kLineCapacity = kMaxDrawnLights * ( kCircleSegments + kMarkLines );
	const int kBatchCapacity = 2; // 線のまとまり 1 つ＋余裕
}

void LightDebugDrawer::draw( const LightList& lights, const Matrix44& projectionView ) {
	if ( !mRenderer ) {
		mRenderer = Scene::PrimitiveRenderer::create( kLineCapacity * kVerticesPerLine, kBatchCapacity );
	}
	mRenderer.setTransform( projectionView );
	mRenderer.enableDepthTest( false ); // 建物の向こうの街灯も見せる
	mRenderer.enableDepthWrite( false );
	mRenderer.setBlendMode( Graphics::BLEND_LINEAR );
	mRenderer.setCullMode( Graphics::CULL_NONE );
	int drawn = 0;
	const std::vector< LightSource >& sources = lights.lights();
	for ( int i = 0; i < static_cast< int >( sources.size() ) && drawn < kMaxDrawnLights; ++i ) {
		const LightSource& light = sources[ i ];
		if ( light.kind == LightKind::Directional ) {
			continue; // 月はどこにも置き場所が無い
		}
		unsigned color = lights.isSelected( i ) ? kLitColor : kDroppedColor;
		const Vector3& p = light.position;
		mRenderer.addLine( Vector3( p.x - kMarkSize, p.y, p.z ), Vector3( p.x + kMarkSize, p.y, p.z ), color, color );
		mRenderer.addLine( Vector3( p.x, p.y - kMarkSize, p.z ), Vector3( p.x, p.y + kMarkSize, p.z ), color, color );
		mRenderer.addLine( Vector3( p.x, p.y, p.z - kMarkSize ), Vector3( p.x, p.y, p.z + kMarkSize ), color, color );
		float lineY = kGroundHeight + kLineLift;
		mRenderer.addLine( p, Vector3( p.x, lineY, p.z ), color, color );
		if ( light.kind == LightKind::Spot ) {
			Vector3 tip;
			tip.setMadd( p, light.direction, kSpotLineLength );
			mRenderer.addLine( p, tip, color, color );
		}
		// 届く球が地面を切る円（光が地面より届く距離以上高ければ、地面には届かないので描かない）。
		float height = p.y - kGroundHeight;
		if ( height < light.range ) {
			float radius = sqrt( light.range * light.range - height * height );
			for ( int s = 0; s < kCircleSegments; ++s ) {
				float a0 = kFullTurnDegree * s / kCircleSegments;
				float a1 = kFullTurnDegree * ( s + 1 ) / kCircleSegments;
				mRenderer.addLine( Vector3( p.x + radius * cos( a0 ), lineY, p.z + radius * sin( a0 ) ),
					Vector3( p.x + radius * cos( a1 ), lineY, p.z + radius * sin( a1 ) ), color, color );
			}
		}
		++drawn;
	}
	mRenderer.draw();
}
