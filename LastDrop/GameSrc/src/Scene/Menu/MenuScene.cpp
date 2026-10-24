#include "GameSrc/Scene/Menu/MenuScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Core/Pad.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"

MenuScene::MenuScene( GameResources* resources, const MenuDefinition& definition ) :
	mPages( &resources->uiRenderer(), &resources->settings(), &resources->audio(), definition ) {
	// 本編でロックしたカーソルを戻す（ボタンをマウスで押せるように）。
	GameLib::Input::Manager::instance().setCursorLocked( false );
}

void MenuScene::beginFrame() {
	// ESC は 1 ページ戻る（最初のページでは何もしない。タイトルを ESC で閉じてしまわないように）。
	if ( InputMenu() ) {
		mPages.back();
	}
	mPages.handleInput();
}

void MenuScene::update() {
	// 時間で進むものは無い（ボタンの操作は入力を読んだ beginFrame で済ませる）。
}

void MenuScene::draw() {
	mPages.draw();
}
