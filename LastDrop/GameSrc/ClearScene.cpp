#include "ClearScene.h"
#include "SceneManager.h"
#include "Pad.h"
#include "UiLabel.h"
#include "UiButton.h"
#include "VertexColor.h"

#include "GameLib/Input/Manager.h"
#include <memory>

using namespace GameLib;

namespace{
	const char* kHeading = "ESCAPED";
	const unsigned kHeadingColor = VertexColor::pack( 0.4f, 0.9f, 0.4f );
	// ボタンの並び（上から）。UiScreen::menuButtonRect の番号。
	const int kRetryButtonIndex = 0;
	const int kTitleButtonIndex = 1;
}

ClearScene::ClearScene() {
	// 本編でロックしたカーソルを戻す（ボタンをマウスで押せるように）。
	GameLib::Input::Manager::instance().setCursorLocked( false );

	UiCanvas& canvas = mScreen.canvas();
	canvas.add( std::make_unique< UiLabel >( kHeading, UiScreen::headingPosition(), UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor ) );
	canvas.add( std::make_unique< UiButton >( UiScreen::menuButtonRect( kRetryButtonIndex ), "Retry", []() {
		SceneManager::instance().requestChange( SceneManager::kPlayScene );
	} ) );
	canvas.add( std::make_unique< UiButton >( UiScreen::menuButtonRect( kTitleButtonIndex ), "Title", []() {
		SceneManager::instance().requestChange( SceneManager::SCENE_TITLE );
	} ) );
}

void ClearScene::beginFrame() {
	// 入力は1回の描画につき1回だけ読む（Main は読まないので、シーンが自分で読む）。
	Pad::instance()->update();
	mScreen.handleInput();
}

void ClearScene::update() {
	// 時間で進むものは無い（ボタンの操作は入力を読んだ beginFrame で済ませる）。
}

void ClearScene::draw() {
	mScreen.draw();
}
