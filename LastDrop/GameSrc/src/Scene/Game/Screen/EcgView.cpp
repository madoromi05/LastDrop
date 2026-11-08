#include "GameSrc/Scene/Game/Screen/EcgView.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Player/PlayerHealth.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <cmath> 

using GameLib::Math::Vector2;

namespace {
	// 1 拍の形（横 t=0..1、縦 h は基線からの高さ。上が +）
	struct Point { float t; float h; };
	const Point kBeat[] = {
	{ 0.00f,  0.00f }, { 0.10f,  0.00f },
	{ 0.15f,  0.12f }, { 0.20f,  0.00f }, // P 波（小さな山）
	{ 0.30f,  0.00f },
	{ 0.33f, -0.10f }, // Q（小さく下）
	{ 0.36f,  1.00f }, // R（鋭く上）
	{ 0.39f, -0.30f }, // S（下）
	{ 0.43f,  0.00f }, { 0.60f,  0.00f },
	{ 0.70f,  0.24f }, { 0.80f,  0.00f }, // T 波（丸い山）
	{ 1.00f,  0.00f }
	};
	const int kBeatPoints = sizeof( kBeat ) / sizeof( kBeat[0] );

	// 波形の見た目
	const float kThickness = 2.4f; // 線の太さ（画素）
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

	// a→b（x は左→右の前提）を area の左右 [left,right] で水平に切ってから積む。
	// 線を引く命令にクリップが無いので、はみ出す端は線上の点を内分して境界へ寄せる（パネルからはみ出させない）。
	void addClippedLine( UiRenderer* renderer, const Vector2& a, const Vector2& b, float left, float right, unsigned color ) {
		const float span = b.x - a.x;
		if (span <= 0.f) {
			return; // 同じ x（拍のつなぎ目など）は積まない
		}
		if (b.x < left || a.x > right) {
			return; // 全部 area の外
		}
		float x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
		if (x0 < left)  { y0 = a.y + ( b.y - a.y ) * ( left  - a.x ) / span; x0 = left; }
		if (x1 > right) { y1 = a.y + ( b.y - a.y ) * ( right - a.x ) / span; x1 = right; }
		renderer->addLine( Vector2( x0, y0 ), Vector2( x1, y1 ), kThickness, color );
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

	// 波線。等間隔サンプリングではなく、1拍の制御点をそのまま画素座標へ置いて線で結ぶ。
	// こうすると鋭い R 波の頂点が必ず頂点として描かれ、スクロールしても高さがブレない（アンダーサンプリングを避ける）。
	const float baseline = area.top + area.height * kHalf;
	const float beatWidth = area.width / style.beatCount;
	const float left = area.left;
	const float right = area.left + area.width;

	// 実時間（ミリ秒）から横ずらし量を出し、右→左へ流す。1 時間でひとまわり（大きい値での精度落ちを避ける）。
	const float seconds = ( GameLib::Framework::instance().time() % 3600000u ) * 0.001f;
	const float scroll = seconds * style.speed; // 画素

	// area に掛かる拍の範囲（端で切れる拍も拾えるよう前後に1拍ずつ余裕を持たせる）。
	const int beatBegin = static_cast< int >( floorf( ( left  + scroll ) / beatWidth ) ) - 1;
	const int beatEnd   = static_cast< int >( floorf( ( right + scroll ) / beatWidth ) ) + 1;

	bool hasPrev = false;
	Vector2 prev( 0.f, 0.f );
	for (int beat = beatBegin; beat <= beatEnd; ++beat) {
		for (int i = 0; i < kBeatPoints; ++i) {
			// 波形座標（拍の通し位置）→ 画面 x。scroll が増えると左へ流れる。
			const float waveX = ( static_cast< float >( beat ) + kBeat[ i ].t ) * beatWidth;
			const Vector2 cur( waveX - scroll, baseline - kBeat[ i ].h * style.amplitude );
			if (hasPrev) {
				addClippedLine( mRenderer, prev, cur, left, right, style.color );
			}
			// つなぎ目を丸くつないで隙間を消す（線の太さと同じ直径）。パネル内の点だけに置く（端の外ははみ出すので置かない）。
			if (cur.x >= left && cur.x <= right) {
				mRenderer->addDisc( cur, kThickness * kHalf, style.color );
			}
			prev = cur;
			hasPrev = true;
		}
	}
}