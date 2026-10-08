#include "GameScene.h"
#include "SceneManager.h"
#include "Pad.h"
#include "UiLabel.h"
#include "UiButton.h"
#include "VertexColor.h"

#include "GameLib/Input/Manager.h"
#include <memory>

using namespace GameLib;

namespace{
	// 段階1の仮の中身（段階2で消す）。
	const char* kPlaceholderHeading = "GAME (WIP)";
	const unsigned kPlaceholderColor = VertexColor::pack( 0.8f, 0.8f, 0.8f );
	const int kClearButtonIndex = 0;
}

GameScene::GameScene() {
	GameLib::Input::Manager::instance().setCursorLocked( false );

	UiCanvas& canvas = mScreen.canvas();
	canvas.add( std::make_unique< UiLabel >( kPlaceholderHeading, UiScreen::headingPosition(), UI_TEXT_LARGE, UI_ALIGN_CENTER, kPlaceholderColor ) );
	canvas.add( std::make_unique< UiButton >( UiScreen::menuButtonRect( kClearButtonIndex ), "Clear (debug)", []() {
		SceneManager::instance().requestChange( SceneManager::SCENE_CLEAR );
	} ) );
}

void GameScene::beginFrame() {
	// 入力は1回の描画につき1回だけ読む（Main は読まないので、シーンが自分で読む）。
	Pad::instance()->update();
	mScreen.handleInput();
}

void GameScene::update() {
}

void GameScene::draw() {
	mScreen.draw();
}
