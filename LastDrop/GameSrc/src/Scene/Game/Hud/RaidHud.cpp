#include "GameSrc/Scene/Game/Hud/RaidHud.h"
#include "GameSrc/Raid/RaidSession.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/Raid/DayCondition.h"
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
	const float kNoticeTop = 70.f;     // 始めの知らせ（1 行目）の真ん中の高さ
	const float kNoticeLineHeight = 30.f;
	// 始めの知らせの何行目に置くか（0 から数える）。今回の条件（大きい字）→ 脱出の車。
	const float kConditionLine = 0.f;
	const float kExitsLine = 1.f;
	const float kExtractFromBottom = 120.f; // 「EXTRACTING」の真ん中の、画面の下端からの高さ

	const unsigned kNoticeColor = VertexColor::pack( 0.95f, 0.95f, 0.95f );
	const unsigned kExitColor = VertexColor::pack( 0.4f, 0.95f, 0.45f ); // 脱出の知らせの色（緑）

	const char* kExitsPrefix = "脱出の車：";
	// 給油し終えた後、車のそばの円の外にいる間。
	const char* kEngineRunningText = "エンジンがかかった：車のそばで待つ";
	const char* kExitsSeparator = " / ";
	const char* kExtractFormat = "脱出中 %.1f";

	// 漁れる場所を開けている間の進み具合（画面の真ん中の少し下。輪の代わりに横の棒）。
	const char* kSearchText = "探している";
	const char* kRefuelText = "給油中"; // 脱出の車に給油している間
	const float kSearchBarBelowCenter = 40.f; // 棒の真ん中の、画面の真ん中からの下がり
	const float kSearchBarWidth = 120.f;
	const float kSearchBarHeight = 6.f;
	const float kSearchTextGap = 16.f;        // 棒の真ん中から、文字の真ん中まで（上へ）
	const unsigned kSearchBackColor = VertexColor::pack( 0.f, 0.f, 0.f, 0.6f );
	const unsigned kSearchFillColor = VertexColor::pack( 0.95f, 0.85f, 0.3f ); // 選んだ枠と同じ黄
	const float kHalf = 0.5f;
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

void RaidHud::draw( const RaidSession& session, const DayConditionEffect& condition, float progress, ProgressKind progressKind ) {
	const RaidParams& params = raidParams();
	char text[ kTextSize ];
	float center = UiLayout::anchorX( UI_ANCHOR_CENTER );
	float height = static_cast< float >( Framework::instance().height() );

	// 始めの数秒だけ、今回の条件・脱出の車の場所。
	if ( session.clock().elapsedSeconds() < params.exitNoticeSeconds ) {
		mRenderer->addText( condition.title, Vector2( center, kNoticeTop + kNoticeLineHeight * kConditionLine ), UI_TEXT_LARGE, UI_ALIGN_CENTER, kNoticeColor );
		std::string exits = kExitsPrefix + exitNames( session.extraction(), kExitsSeparator );
		mRenderer->addText( exits.c_str(), Vector2( center, kNoticeTop + kNoticeLineHeight * kExitsLine ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kExitColor );
	}

	// 給油し終えた後：車のそばの円の中にいる間は脱出までの残り、外にいる間は戻るように知らせる。
	if ( session.extraction().isInside() ) {
		std::snprintf( text, sizeof( text ), kExtractFormat, session.extraction().remainingSeconds() );
		mRenderer->addText( text, Vector2( center, height - kExtractFromBottom ), UI_TEXT_LARGE, UI_ALIGN_CENTER, kExitColor );
	} else if ( session.extraction().isOpen() ) {
		mRenderer->addText( kEngineRunningText, Vector2( center, height - kExtractFromBottom ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kExitColor );
	}

	// 漁れる場所を開けている間・給油している間（取りやめ・終わりで消える）。
	if ( progress >= 0.f ) {
		float barMiddle = height * kHalf + kSearchBarBelowCenter;
		UiRect back;
		back.left = center - kSearchBarWidth * kHalf;
		back.top = barMiddle - kSearchBarHeight * kHalf;
		back.width = kSearchBarWidth;
		back.height = kSearchBarHeight;
		mRenderer->addRect( back, kSearchBackColor );
		UiRect fill = back;
		fill.width = kSearchBarWidth * progress;
		mRenderer->addRect( fill, kSearchFillColor );
		const char* label = ( progressKind == ProgressKind::Refuel ) ? kRefuelText : kSearchText;
		mRenderer->addText( label, Vector2( center, barMiddle - kSearchTextGap ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kNoticeColor );
	}
	mRenderer->draw();
}
