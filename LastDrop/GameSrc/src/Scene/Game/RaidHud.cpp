#include "GameSrc/Scene/Game/RaidHud.h"
#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/UI/UiRenderer.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "Framework.h"
#include <cmath>
#include <cstdio>

using namespace GameLib;
using namespace GameLib::Math;

namespace{
	const int kTextSize = 64;
	const int kSecondsPerMinute = 60;
	const float kTwice = 2.f;          // 画面の右端 = 中央の 2 倍
	// 右上の残り時間。文字の幅は外から測れないので、真ん中揃えで右端から決まった所に置く（「09:42」が収まる長さ）。
	const float kClockFromRight = 56.f;
	const float kClockTop = 26.f;
	const float kNoticeTop = 70.f;     // 始めの知らせ（日）の真ん中の高さ
	const float kNoticeLineHeight = 30.f;
	const float kExtractFromBottom = 120.f; // 「EXTRACTING」の真ん中の、画面の下端からの高さ

	const unsigned kClockColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
	const unsigned kWarningColor = VertexColor::pack( 0.95f, 0.25f, 0.2f );
	const unsigned kNoticeColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
	const unsigned kExitColor = VertexColor::pack( 0.4f, 0.95f, 0.45f ); // 脱出点の光と同じ緑の仲間

	const char* kDayFormat = "DAY %d";
	const char* kExitsPrefix = "脱出点：";
	const char* kExitsSeparator = " / ";
	const char* kExtractFormat = "脱出中 %.1f";
}

RaidHud::RaidHud( UiRenderer* renderer ) :
	mRenderer( renderer ) {
	ASSERT( mRenderer && "RaidHud : renderer is null." );
}

std::string RaidHud::exitNames( const ExtractionTracker& extraction, const char* separator ) {
	std::string names;
	for ( const ExtractZone& zone : extraction.zones() ) {
		if ( !names.empty() ) {
			names += separator;
		}
		names += zone.name;
	}
	return names;
}

void RaidHud::draw( const RaidSession& session, int day ) {
	const RaidParams& params = raidParams();
	char text[ kTextSize ];
	float center = UiLayout::anchorX( UI_ANCHOR_CENTER );
	float height = static_cast< float >( Framework::instance().height() );

	// 右上の残り時間（秒は切り上げ。0:00 になった時が時間切れ）。
	float remaining = session.clock().remainingSeconds();
	int seconds = static_cast< int >( std::ceil( remaining ) );
	std::snprintf( text, sizeof( text ), "%02d:%02d", seconds / kSecondsPerMinute, seconds % kSecondsPerMinute );
	Vector2 clock( center * kTwice - kClockFromRight, kClockTop );
	mRenderer->addText( text, clock, UI_TEXT_LARGE, UI_ALIGN_CENTER, ( remaining <= params.warningSeconds ) ? kWarningColor : kClockColor );

	// 始めの数秒だけ、何日目かと開いている脱出点。
	if ( session.clock().elapsedSeconds() < params.exitNoticeSeconds ) {
		std::snprintf( text, sizeof( text ), kDayFormat, day );
		mRenderer->addText( text, Vector2( center, kNoticeTop ), UI_TEXT_LARGE, UI_ALIGN_CENTER, kNoticeColor );
		std::string exits = kExitsPrefix + exitNames( session.extraction(), kExitsSeparator );
		mRenderer->addText( exits.c_str(), Vector2( center, kNoticeTop + kNoticeLineHeight ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kExitColor );
	}

	// 脱出点の円の中にいる間、脱出までの残り。
	if ( session.extraction().isInside() ) {
		std::snprintf( text, sizeof( text ), kExtractFormat, session.extraction().remainingSeconds() );
		mRenderer->addText( text, Vector2( center, height - kExtractFromBottom ), UI_TEXT_LARGE, UI_ALIGN_CENTER, kExitColor );
	}
	mRenderer->draw();
}
