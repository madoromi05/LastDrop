#include "GameSrc/Scene/Result/GameOverPage.h"
#include "GameSrc/Scene/Result/ResultText.h"
#include "GameSrc/Raid/RaidResult.h"
#include "GameSrc/Render/VertexColor.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/MenuPalette.h"
#include "GameSrc/UI/Widget/UiBackdrop.h"
#include "GameSrc/UI/Widget/UiLabel.h"
#include "GameSrc/UI/Widget/UiBar.h"
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/Widget/UiKeyHintRow.h"

#include "GameLib/GameLib.h"
#include <memory>
#include <string>
#include <vector>

namespace{
	const char* kHeading = "GAME OVER";
	const char* kElapsedName = "経過";
	const char* kKillsName = "倒した数";
	const char* kNameToValue = " ";			// 名前と値の間
	const char* kItemSeparator = "　・　";	// 「経過 08:42」と「倒した数 12」の間
	const char* kRetryLabel = "もう一度";
	const char* kToTitleLabel = "タイトルへ";
	// 字の間（画素）。文字の高さに対する割合をモックから写した
	const float kHeadingLetterSpacing = 10.6f;	// GAME OVER（文字の高さの 0.22 倍くらい）
	const float kSummaryLetterSpacing = 2.4f;	// 経過と倒した数の 1 行（文字の高さの 0.2 倍くらい）
	const char* kBackdrop = "Resource/UI/gameover_backdrop.tga";	// 黒地の端に暗い赤がにじむ絵（tools/texgen/gameover_backdrop_gen.py）。読めなければ黒のまま
	const float kBackdropAlignX = 0.5f;								// 左右が同じ絵なので、真ん中同士で合わせる
	const int kChoiceCount = 2;										// 横に並べる行の数（もう一度・タイトルへ）

	// 画面の下の操作の案内。行が横に並ぶので、選ぶのは左右
	const std::vector< UiKeyHintRow::Hint > kKeyHints = {
		{ "← →", "選ぶ" },
		{ "Enter", "決定" },
	};

	// 経過と倒した数の 1 行（「経過 08:42　・　倒した数 12」）
	std::string summaryOf( const RaidResult& result ) {
		return std::string( kElapsedName ) + kNameToValue + ResultText::time( result.seconds )
			+ kItemSeparator + kKillsName + kNameToValue + std::to_string( result.kills );
	}
}

void GameOverPage::build( UiCanvas* canvas, const RaidResult& result, std::function< void() > retry, std::function< void() > toTitle ) {
	ASSERT( canvas && retry && toTitle && "GameOverPage : canvas is null or a callback is empty." );
	// 足した順に描くので、背景を先に足す
	canvas->add( std::make_unique< UiBackdrop >( kBackdrop, VertexColor::WHITE, kBackdropAlignX ) );
	canvas->add( std::make_unique< UiLabel >( kHeading, UI_ANCHOR_CENTER, UiScreen::centerHeadingPosition(), UI_TEXT_TITLE, UI_ALIGN_CENTER, MenuPalette::DEATH_HEADING, kHeadingLetterSpacing ) );
	canvas->add( std::make_unique< UiBar >( UI_ANCHOR_CENTER, UiScreen::centerRuleRect(), MenuPalette::DEATH_LINE ) );
	canvas->add( std::make_unique< UiLabel >( summaryOf( result ).c_str(), UI_ANCHOR_CENTER, UiScreen::centerSummaryPosition(), UI_TEXT_CAPTION, UI_ALIGN_CENTER, MenuPalette::TEXT_SUB, kSummaryLetterSpacing ) );
	// 行は左から足す（← → で選ぶ順。始めは「もう一度」が選ばれる）
	canvas->add( std::make_unique< UiMenuItem >( UI_ANCHOR_CENTER, UiScreen::centerChoiceRect( 0, kChoiceCount ), kRetryLabel, retry, UiMenuItem::kJapaneseSpacing, UI_ALIGN_CENTER ) );
	canvas->add( std::make_unique< UiMenuItem >( UI_ANCHOR_CENTER, UiScreen::centerChoiceRect( 1, kChoiceCount ), kToTitleLabel, toTitle, UiMenuItem::kJapaneseSpacing, UI_ALIGN_CENTER ) );
	canvas->add( std::make_unique< UiKeyHintRow >( UI_ANCHOR_CENTER, UiScreen::centerKeyHintPosition(), kKeyHints, UI_ALIGN_CENTER ) );
}
