#include "GameSrc/Scene/Game/Screen/EcgView.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Player/PlayerHealth.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <cmath> 

namespace {
	// 1 拍の形（横 t=0..1、縦 h は基線からの高さ。上が +）
	struct Point { float t; float h; };
	const Point kBeat[] = {
		{ 0.00f, 0.f }, { 0.42f, 0.f }, { 0.46f, -0.15f }, { 0.50f, 1.0f },
		{ 0.54f, -0.28f }, { 0.58f, 0.f }, { 1.00f, 0.f }
	};
	const int kBeatPoints = sizeof( kBeat ) / sizeof( kBeat[0] );

	// t（0..1）における拍の高さ。制御点を直線でつなぐ。
	float beatHeight( float t ) {
		for (int i = 1; i < kBeatPoints; ++i) {
			if (t <= kBeat[i].t) {
				const Point& a = kBeat[i - 1];
				const Point& b = kBeat[i];
				const float span = b.t - a.t;
				const float u = ( span > 0.f ) ? ( t - a.t ) / span : 0.f;
				return a.h + ( b.h - a.h ) * u;
			}
		}
		return 0.f;
	}

	// 波形の見た目
	const float kThickness = 2.4f; // 線の太さ（画素）
	const float kStep = 2.f;       // x を進める刻み（画素。小さいほど滑らか）
	const float kHalf = 0.5f;

	struct EcgStyle {
		unsigned color;
		float amplitude; // トゲの高さ（画素）
		int beatCount;   // area に入れる拍の数
		float speed;     // 横スクロールの速さ（画素/秒）。大きいほど速く流れる
	};
	EcgStyle styleFor( PlayerCondition condition ) {
		switch (condition) {
		case PlayerCondition::Caution:
			return {VertexColor::pack( 0.88f, 0.66f, 0.17f ), 15.f, 3, 48.f}; // 黄オレンジ・低め・やや遅い
		case PlayerCondition::Danger:
			return {VertexColor::pack( 0.88f, 0.23f, 0.18f ), 10.f, 2, 34.f}; // 赤・弱く平らが長い・遅い
		case PlayerCondition::Fine:
		default:
			return {VertexColor::pack( 0.23f, 0.82f, 0.41f ), 22.f, 4, 60.f}; // 緑・規則正しく高い・普通
		}
	}
}

EcgView::EcgView( UiRenderer* renderer ) :
	mRenderer( renderer ) {
	ASSERT( mRenderer && "EcgView : renderer is null." );
}

void EcgView::draw( const UiRect& area, PlayerCondition condition ) {
	const EcgStyle style = styleFor( condition );

	// 背景の板（半透明。後ろの町を少し透かす）。
	const unsigned kPanelColor = VertexColor::pack( 0.f, 0.f, 0.f, 0.35f );
	mRenderer->addRect( area, kPanelColor );

	// 波線。線を引く命令が無いので、縦長の長方形を横に並べて折れ線にする。
	const float baseline = area.top + area.height * kHalf;
	const float beatWidth = area.width / style.beatCount;

	// 実時間（ミリ秒）から横ずらし量を出し、右→左へ流す。1 時間でひとまわり（大きい値での精度落ちを避ける）。
	const float seconds = ( GameLib::Framework::instance().time() % 3600000u ) * 0.001f;
	const float scroll = seconds * style.speed; // 画素

	float prevLocalX = 0.f;
	float prevY = baseline - beatHeight( fmodf( scroll, beatWidth ) / beatWidth ) * style.amplitude;
	for (float localX = kStep; localX <= area.width; localX += kStep) {
		const float t = fmodf( localX + scroll, beatWidth ) / beatWidth; // 時間ぶんずらした位相（0..1）
		const float y = baseline - beatHeight( t ) * style.amplitude;

		// 前の点と今の点の y の間を、太さぶん膨らませた縦長の長方形で埋める。
		UiRect seg;
		seg.left = area.left + prevLocalX;
		seg.width = ( localX - prevLocalX ) + kThickness;
		seg.top = ( ( prevY < y ) ? prevY : y ) - kThickness * kHalf;
		seg.height = fabsf( y - prevY ) + kThickness;
		mRenderer->addRect( seg, style.color );

		prevLocalX = localX;
		prevY = y;
	}
}