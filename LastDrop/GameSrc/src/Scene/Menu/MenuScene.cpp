#include "GameSrc/Scene/Menu/MenuScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/UI/UiLabel.h"
#include "GameSrc/UI/UiButton.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include <memory>

MenuScene::MenuScene( GameResources* resources, const MenuDefinition& definition ) :
	mScreen( &resources->uiRenderer() ) {
	ASSERT( definition.heading && "MenuScene : heading is null." );
	// 本編でロックしたカーソルを戻す（ボタンをマウスで押せるように）。
	GameLib::Input::Manager::instance().setCursorLocked( false );

	UiCanvas& canvas = mScreen.canvas();
	canvas.add( std::make_unique< UiLabel >( definition.heading, UI_ANCHOR_CENTER, UiScreen::headingPosition(), UI_TEXT_LARGE, UI_ALIGN_CENTER, definition.headingColor ) );
	int buttonIndex = 0; // 上から何番目か（UiScreen::menuButtonRect の番号）
	for ( const MenuDefinition::Button& button : definition.buttons ) {
		canvas.add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, UiScreen::menuButtonRect( buttonIndex ), button.label, button.onActivate ) );
		++buttonIndex;
	}
}

void MenuScene::beginFrame() {
	mScreen.handleInput();
}

void MenuScene::update() {
	// 時間で進むものは無い（ボタンの操作は入力を読んだ beginFrame で済ませる）。
}

void MenuScene::draw() {
	mScreen.draw();
}
