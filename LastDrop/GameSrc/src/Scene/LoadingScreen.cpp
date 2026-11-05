#include "GameSrc/Scene/LoadingScreen.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiRect.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <cstdio>

using namespace GameLib::Math;

namespace{
	const int kTextSize = 32;
	const char* kDayFormat = "DAY %d"; // 町の始めの知らせ（RaidHud）と同じ書き方
	const float kHalf = 0.5f;
	// 画面の真ん中からの高さ（上が負）。
	const float kDayAboveCenter = 40.f;
	const float kLabelBelowCenter = 10.f;
	const float kBarBelowCenter = 40.f;
	const float kBarWidth = 240.f;
	const float kBarHeight = 8.f;
	const float kBarFrame = 1.f; // 棒の枠の太さ
	const float kBothSides = 2.f; // 枠は棒の両側に付く

	const unsigned kBackColor = VertexColor::pack( 0.f, 0.f, 0.f );
	const unsigned kTextColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
	const unsigned kFrameColor = VertexColor::pack( 0.6f, 0.6f, 0.6f );
	const unsigned kBarBackColor = VertexColor::pack( 0.12f, 0.12f, 0.12f );
	const unsigned kBarFillColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
}

LoadingScreen::LoadingScreen( UiRenderer* renderer ) :
	mRenderer( renderer ) {
	ASSERT( mRenderer && "LoadingScreen : renderer is null." );
}

void LoadingScreen::draw( int day, const char* label, float progress ) {
	float width = static_cast< float >( GameLib::Framework::instance().width() );
	float height = static_cast< float >( GameLib::Framework::instance().height() );
	float centerX = width * kHalf;
	float centerY = height * kHalf;

	// 前の画面（倉庫）が残らないよう、画面全体を黒で塗る（世界はまだ描いていない）。
	UiRect back = { 0.f, 0.f, width, height };
	mRenderer->addRect( back, kBackColor );

	char text[ kTextSize ];
	std::snprintf( text, sizeof( text ), kDayFormat, day );
	mRenderer->addText( text, Vector2( centerX, centerY - kDayAboveCenter ), UI_TEXT_LARGE, UI_ALIGN_CENTER, kTextColor );
	mRenderer->addText( label, Vector2( centerX, centerY + kLabelBelowCenter ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor );

	float ratio = ( progress < 0.f ) ? 0.f : ( ( progress > 1.f ) ? 1.f : progress );
	UiRect frame = { centerX - kBarWidth * kHalf - kBarFrame, centerY + kBarBelowCenter - kBarFrame,
		kBarWidth + kBarFrame * kBothSides, kBarHeight + kBarFrame * kBothSides };
	mRenderer->addRect( frame, kFrameColor );
	UiRect bar = { centerX - kBarWidth * kHalf, centerY + kBarBelowCenter, kBarWidth, kBarHeight };
	mRenderer->addRect( bar, kBarBackColor );
	bar.width = kBarWidth * ratio;
	mRenderer->addRect( bar, kBarFillColor );
	mRenderer->draw();
}
