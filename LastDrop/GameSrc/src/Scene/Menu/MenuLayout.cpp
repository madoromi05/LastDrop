#include "GameSrc/Scene/Menu/MenuLayout.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/UiLabel.h"
#include "GameSrc/UI/UiButton.h"

#include "GameLib/GameLib.h"
#include <memory>

void MenuLayout::build( UiCanvas* canvas, const MenuDefinition& definition ) {
	ASSERT( canvas && "MenuLayout : canvas is null." );
	ASSERT( definition.heading && "MenuLayout : heading is null." );
	canvas->add( std::make_unique< UiLabel >( definition.heading, UI_ANCHOR_CENTER, UiScreen::headingPosition(), UI_TEXT_LARGE, UI_ALIGN_CENTER, definition.headingColor ) );
	int buttonIndex = 0; // 上から何番目か（UiScreen::menuButtonRect の番号）
	for ( const MenuDefinition::Button& button : definition.buttons ) {
		canvas->add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, UiScreen::menuButtonRect( buttonIndex ), button.label, button.onActivate ) );
		++buttonIndex;
	}
}
