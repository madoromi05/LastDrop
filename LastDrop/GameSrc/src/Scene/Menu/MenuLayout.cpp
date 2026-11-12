#include "GameSrc/Scene/Menu/MenuLayout.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/MenuPalette.h"
#include "GameSrc/UI/Widget/UiLabel.h"
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/Widget/UiBackdrop.h"
#include "GameSrc/UI/Widget/UiShade.h"
#include "GameSrc/UI/Widget/UiBar.h"
#include "GameSrc/UI/Widget/UiKeyHintRow.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include <memory>

namespace{
	// 字の間と影の範囲。位置は UiScreen、色は MenuPalette が持つ。
	const float kTitleLetterSpacing = 6.f;   // 題名の字の間（文字の高さの 0.12 倍くらい。広げすぎると背景の街灯にかかる）
	const float kTaglineLetterSpacing = 2.4f; // ひと言の字の間（文字の高さの 0.2 倍くらい）
	const float kHeadingLetterSpacing = 6.4f; // 設定・操作の一覧の見出しの字の間（文字の高さの 0.2 倍くらい）
	// 背景の絵の左を暗くする範囲（画面の幅に対する割合）。題名とメニューの行が収まる所までは濃いまま、そこから薄れる。
	const float kShadeSolidUntil = 0.22f;
	const float kShadeFadeUntil = 0.62f;
	// 画面の下の操作の案内。操作は UiCanvas の決まり（Enter / Space で決定、ESC で 1 ページ戻る。MenuScene）。
	const std::vector< UiKeyHintRow::Hint > kKeyHints = {
		{ "Enter", "決定" },
		{ "Esc", "戻る" },
	};

	// ボタンの決定の処理。ページを開くボタンなら、そのページを開く処理にする。
	std::function< void() > activateOf( const MenuDefinition::Button& button, const std::function< void( MenuPageId ) >& openPage ) {
		if ( button.opens == MENU_PAGE_NONE ) {
			return button.onActivate;
		}
		ASSERT( openPage && "MenuLayout : openPage is empty." );
		MenuPageId page = button.opens;
		return [ openPage, page ]() { openPage( page ); };
	}
}

// 背景の絵（左を暗く）→ 題名・赤い線・ひと言 → 板なしの行 → 下の案内。足した順に描くので、背景を先に足す。
void MenuLayout::build( UiCanvas* canvas, const MenuDefinition& definition, std::function< void( MenuPageId ) > openPage ) {
	ASSERT( canvas && "MenuLayout : canvas is null." );
	ASSERT( definition.heading && "MenuLayout : heading is null." );
	if ( definition.backdrop ) {
		canvas->add( std::make_unique< UiBackdrop >( definition.backdrop, VertexColor::WHITE, definition.backdropAlignX ) );
		canvas->add( std::make_unique< UiShade >( MenuPalette::SHADE, kShadeSolidUntil, kShadeFadeUntil ) );
	}
	canvas->add( std::make_unique< UiLabel >( definition.heading, UI_ANCHOR_LEFT, UiScreen::sideTitlePosition(), UI_TEXT_TITLE, UI_ALIGN_LEFT, MenuPalette::TEXT, kTitleLetterSpacing ) );
	canvas->add( std::make_unique< UiBar >( UI_ANCHOR_LEFT, UiScreen::sideRuleRect(), MenuPalette::ACCENT ) );
	if ( definition.tagline ) {
		canvas->add( std::make_unique< UiLabel >( definition.tagline, UI_ANCHOR_LEFT, UiScreen::sideTaglinePosition(), UI_TEXT_CAPTION, UI_ALIGN_MIDDLE_LEFT, MenuPalette::TEXT_SUB, kTaglineLetterSpacing ) );
	}
	const int itemCount = static_cast< int >( definition.buttons.size() );
	int itemIndex = 0; // 上から何番目か（UiScreen::sideMenuItemRect の番号）
	for ( const MenuDefinition::Button& button : definition.buttons ) {
		canvas->add( std::make_unique< UiMenuItem >( UI_ANCHOR_LEFT, UiScreen::sideMenuItemRect( itemIndex, itemCount ), button.label, activateOf( button, openPage ), UiMenuItem::kLatinSpacing ) );
		++itemIndex;
	}
	canvas->add( std::make_unique< UiKeyHintRow >( UI_ANCHOR_LEFT, UiScreen::keyHintPosition(), kKeyHints ) );
}

void MenuLayout::buildSideHeading( UiCanvas* canvas, const char* heading ) {
	ASSERT( canvas && heading && "MenuLayout : canvas or heading is null." );
	canvas->add( std::make_unique< UiLabel >( heading, UI_ANCHOR_LEFT, UiScreen::sideHeadingPosition(), UI_TEXT_HEADING, UI_ALIGN_LEFT, MenuPalette::TEXT, kHeadingLetterSpacing ) );
	canvas->add( std::make_unique< UiBar >( UI_ANCHOR_LEFT, UiScreen::sideHeadingRuleRect(), MenuPalette::ACCENT ) );
}
