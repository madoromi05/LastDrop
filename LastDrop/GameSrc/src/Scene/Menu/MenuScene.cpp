#include "GameSrc/Scene/Menu/MenuScene.h"
#include "GameSrc/Scene/GameResources.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"

MenuScene::MenuScene( GameResources* resources, const MenuDefinition& definition ) :
	mScreen( &resources->uiRenderer() ) {
	// 本編でロックしたカーソルを戻す（ボタンをマウスで押せるように）。
	GameLib::Input::Manager::instance().setCursorLocked( false );
	MenuLayout::build( &mScreen.canvas(), definition );
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
