#include "GameSrc/Scene/Game/PlayerBodyFigure.h"
#include "GameSrc/Player/PlayerHealth.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"

namespace{
	// ── 置き場所と大きさ（画素。2026-10-21 ユーザー指定：左上に薄く・小さく）──
	// 左上の角から余白をあけて置く（確認用の文字は左下へ移したので、上には何も無い）。
	const float kFigureLeft = 16.f;
	const float kFigureTop = 16.f;
	// 部位の四角の表（下）を何倍して描くか。表は高さ 120 画素で作ってあり、0.75 倍で 90 画素（720 の画面の約 1/8）。
	const float kFigureScale = 0.75f;
	const float kLayoutWidth = 64.f;
	const float kLayoutHeight = 120.f;

	// 部位の四角（図の左上からの画素。kFigureScale を掛ける前）。並びは PlayerBodyZone と同じ（static_assert で数を確かめる）。
	// 正面から見た向き（参考の絵と同じ。自分の左腕が画面の右）。
	const UiRect kZoneRects[] = {
		{ 24.f,  0.f, 16.f, 16.f }, // Head
		{ 18.f, 18.f, 28.f, 28.f }, // Thorax
		{ 18.f, 48.f, 28.f, 20.f }, // Stomach
		{ 48.f, 18.f, 10.f, 44.f }, // LeftArm（正面から見るので、画面の右）
		{  6.f, 18.f, 10.f, 44.f }, // RightArm
		{ 33.f, 70.f, 13.f, 50.f }, // LeftLeg（同じく画面の右）
		{ 18.f, 70.f, 13.f, 50.f }, // RightLeg
	};
	const int kZoneNumber = static_cast< int >( PlayerBodyZone::Count );
	static_assert( sizeof( kZoneRects ) / sizeof( kZoneRects[ 0 ] ) == kZoneNumber, "kZoneRects must cover every PlayerBodyZone." );

	// 四角のふちの線（画素）。半透明の四角どうしの境目と、明るい背景の上での形が分かるように、暗い線で囲む。
	// 四角の下に一回り大きな板を敷くと、半透明なので板が四角の中から透けて見えるため、4 本の細い板で囲む。
	const float kOutline = 1.f;
	const float kOutlineAlpha = 0.35f;
	const float kBothSides = 2.f; // ふちの線は両側（上下・左右）に付く

	// 残りの割合ごとの色。満タン → 半分 → 4 分の 1 の間はなめらかに混ぜ、4 分の 1 より下は赤のまま。
	struct Color{
		float r;
		float g;
		float b;
	};
	const Color kHealthyColor  = { 0.30f, 0.80f, 0.30f }; // 緑
	const Color kHurtColor     = { 0.95f, 0.85f, 0.20f }; // 黄
	const Color kCriticalColor = { 0.85f, 0.15f, 0.10f }; // 赤
	const Color kBrokenColor   = { 0.18f, 0.18f, 0.18f }; // 壊れた（暗い灰色）
	const Color kOutlineColor  = { 0.05f, 0.05f, 0.05f };
	const float kFullRatio = 1.f;
	const float kHurtRatio = 0.5f;
	const float kCriticalRatio = 0.25f;
	// 不透明度。いつも薄く出し、減った部位ほど少し濃くして目に入るようにする（壊れた部位は一番濃い）
	const float kHealthyAlpha = 0.25f;
	const float kHurtAlpha = 0.65f;

	Color mix( const Color& a, const Color& b, float t ) {
		return { a.r + ( b.r - a.r ) * t, a.g + ( b.g - a.g ) * t, a.b + ( b.b - a.b ) * t };
	}

	unsigned zoneColor( const PlayerHealth& health, PlayerBodyZone zone ) {
		if ( health.isBroken( zone ) ) {
			return VertexColor::pack( kBrokenColor.r, kBrokenColor.g, kBrokenColor.b, kHurtAlpha );
		}
		float ratio = health.part( zone ) / health.maxPart( zone );
		Color color = kCriticalColor;
		if ( ratio >= kHurtRatio ) {
			color = mix( kHealthyColor, kHurtColor, ( kFullRatio - ratio ) / ( kFullRatio - kHurtRatio ) );
		} else if ( ratio >= kCriticalRatio ) {
			color = mix( kHurtColor, kCriticalColor, ( kHurtRatio - ratio ) / ( kHurtRatio - kCriticalRatio ) );
		}
		float alpha = kHurtAlpha + ( kHealthyAlpha - kHurtAlpha ) * ratio;
		return VertexColor::pack( color.r, color.g, color.b, alpha );
	}

	// rect の内側のふちに沿って、細い線を 4 本積む（上・下・左・右）。
	void addOutline( const UiRect& rect, UiRenderer* renderer ) {
		unsigned color = VertexColor::pack( kOutlineColor.r, kOutlineColor.g, kOutlineColor.b, kOutlineAlpha );
		float innerHeight = rect.height - kOutline * kBothSides;
		renderer->addRect( { rect.left, rect.top, rect.width, kOutline }, color );
		renderer->addRect( { rect.left, rect.bottom() - kOutline, rect.width, kOutline }, color );
		renderer->addRect( { rect.left, rect.top + kOutline, kOutline, innerHeight }, color );
		renderer->addRect( { rect.right() - kOutline, rect.top + kOutline, kOutline, innerHeight }, color );
	}
}

UiRect PlayerBodyFigure::bounds() {
	UiRect rect;
	rect.left = kFigureLeft;
	rect.top = kFigureTop;
	rect.width = kLayoutWidth * kFigureScale;
	rect.height = kLayoutHeight * kFigureScale;
	return rect;
}

void PlayerBodyFigure::add( const PlayerHealth& health, UiRenderer* renderer ) {
	ASSERT( renderer && "PlayerBodyFigure : renderer is null." );
	UiRect figure = bounds();
	for ( int i = 0; i < kZoneNumber; ++i ) {
		const UiRect& local = kZoneRects[ i ];
		UiRect fill;
		fill.left = figure.left + local.left * kFigureScale;
		fill.top = figure.top + local.top * kFigureScale;
		fill.width = local.width * kFigureScale;
		fill.height = local.height * kFigureScale;
		// 中を塗ってから、ふちの線を内側へ重ねる（線と中が重ならないよう、中はふちの内側だけ）。
		UiRect inner = { fill.left + kOutline, fill.top + kOutline, fill.width - kOutline * kBothSides, fill.height - kOutline * kBothSides };
		renderer->addRect( inner, zoneColor( health, static_cast< PlayerBodyZone >( i ) ) );
		addOutline( fill, renderer );
	}
}
