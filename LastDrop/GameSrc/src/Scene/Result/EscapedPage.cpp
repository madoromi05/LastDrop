#include "GameSrc/Scene/Result/EscapedPage.h"
#include "GameSrc/Scene/Result/ResultText.h"
#include "GameSrc/Raid/RaidResult.h"
#include "GameSrc/Raid/DayCondition.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/MenuPalette.h"
#include "GameSrc/UI/Widget/UiBackdrop.h"
#include "GameSrc/UI/Widget/UiShade.h"
#include "GameSrc/UI/Widget/UiLabel.h"
#include "GameSrc/UI/Widget/UiBar.h"
#include "GameSrc/UI/Widget/UiStatTable.h"
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/Widget/UiKeyHintRow.h"

#include "GameLib/GameLib.h"
#include <memory>
#include <string>
#include <vector>

namespace{
	const char* kLead = "町を出た。";
	const char* kHeading = "ESCAPED";
	const char* kTimeName = "かかった時間";
	const char* kKillsName = "倒した数";
	const char* kConditionName = "今回の条件";
	const char* kToTitleLabel = "タイトルへ";
	// 字の間（画素）。文字の高さに対する割合をモックから写した
	const float kLeadLetterSpacing = 3.6f;		// ひと言（文字の高さの 0.3 倍くらい）
	const float kHeadingLetterSpacing = 9.6f;	// ESCAPED（文字の高さの 0.2 倍くらい）
	// 背景の絵（脱出の車を後ろから写した画面写真）。読めなければ黒のまま
	// 車を画面の真ん中より右に写した絵を、真ん中同士で合わせる（左の文字と車を離す）
	const char* kBackdrop = "Resource/UI/escaped_backdrop.tga";
	const float kBackdropAlignX = 0.5f;
	// 背景の左を暗くする範囲（画面の幅に対する割合）。表がタイトルのメニューより広いので、濃い所も広くする
	const float kShadeSolidUntil = 0.35f;
	const float kShadeFadeUntil = 0.72f;

	// 画面の下の操作の案内。選べる行は「タイトルへ」の 1 つだけなので、選ぶ案内は出さない
	const std::vector< UiKeyHintRow::Hint > kKeyHints = {
		{ "Enter", "決定" },
	};
}

void EscapedPage::build( UiCanvas* canvas, const RaidResult& result, std::function< void() > toTitle ) {
	ASSERT( canvas && toTitle && "EscapedPage : canvas is null or toTitle is empty." );
	// 足した順に描くので、背景を先に足す
	canvas->add( std::make_unique< UiBackdrop >( kBackdrop, VertexColor::WHITE, kBackdropAlignX ) );
	canvas->add( std::make_unique< UiShade >( MenuPalette::SHADE, kShadeSolidUntil, kShadeFadeUntil ) );
	canvas->add( std::make_unique< UiLabel >( kLead, UI_ANCHOR_LEFT, UiScreen::sideLeadPosition(), UI_TEXT_CAPTION, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT_SUB, kLeadLetterSpacing ) );
	canvas->add( std::make_unique< UiLabel >( kHeading, UI_ANCHOR_LEFT, UiScreen::sideTitlePosition(), UI_TEXT_TITLE, UI_ALIGN_LEFT, MenuPalette::TEXT, kHeadingLetterSpacing ) );
	canvas->add( std::make_unique< UiBar >( UI_ANCHOR_LEFT, UiScreen::sideRuleRect(), MenuPalette::ACCENT ) );
	const std::vector< UiStatTable::Row > rows = {
		{ kTimeName, ResultText::time( result.seconds ) },
		{ kKillsName, std::to_string( result.kills ) },
		{ kConditionName, dayConditionEffect( result.condition ).title },
	};
	canvas->add( std::make_unique< UiStatTable >( UI_ANCHOR_LEFT, UiScreen::sideTableRowRect(), rows ) );
	canvas->add( std::make_unique< UiMenuItem >( UI_ANCHOR_LEFT, UiScreen::sideBackItemRect(), kToTitleLabel, toTitle, UiMenuItem::kJapaneseSpacing ) );
	canvas->add( std::make_unique< UiKeyHintRow >( UI_ANCHOR_LEFT, UiScreen::keyHintPosition(), kKeyHints ) );
}
