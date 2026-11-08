#include "GameSrc/Scene/Menu/MenuLayout.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/Widget/UiLabel.h"
#include "GameSrc/UI/Widget/UiButton.h"

#include "GameLib/GameLib.h"
#include <memory>

void MenuLayout::build( UiCanvas* canvas, const MenuDefinition& definition, std::function< void( MenuPageId ) > openPage ) {
	ASSERT( canvas && "MenuLayout : canvas is null." );
	ASSERT( definition.heading && "MenuLayout : heading is null." );
	canvas->add( std::make_unique< UiLabel >( definition.heading, UI_ANCHOR_CENTER, UiScreen::headingPosition(), UI_TEXT_LARGE, UI_ALIGN_CENTER, definition.headingColor ) );
	int buttonIndex = 0; // 上から何番目か（UiScreen::menuButtonRect の番号）
	for ( const MenuDefinition::Button& button : definition.buttons ) {
		std::function< void() > onActivate = button.onActivate;
		if ( button.opens != MENU_PAGE_NONE ) {
			ASSERT( openPage && "MenuLayout : openPage is empty." );
			MenuPageId page = button.opens;
			onActivate = [ openPage, page ]() { openPage( page ); };
		}
		canvas->add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, UiScreen::menuButtonRect( buttonIndex ), button.label, onActivate ) );
		++buttonIndex;
	}
}
