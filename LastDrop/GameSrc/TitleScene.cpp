#include "TitleScene.h"
#include "SceneManager.h"
#include "Pad.h"
#include "UiLabel.h"
#include "UiButton.h"
#include "VertexColor.h"
#include "Framework.h"

#include "GameLib/Input/Manager.h"
#include <memory>

using namespace GameLib;

namespace{
	const char* kGameTitle = "ZOMBIE ESCAPE"; // 仮の名前
	const unsigned kTitleColor = VertexColor::pack( 0.95f, 0.75f, 0.2f );
	// ボタンの並び（上から）。UiScreen::menuButtonRect の番号。
	const int kStartButtonIndex = 0;
	const int kQuitButtonIndex = 1;
}

TitleScene::TitleScene() {
	// 本編でロックしたカーソルを戻す（ボタンをマウスで押せるように）。
	GameLib::Input::Manager::instance().setCursorLocked( false );

	UiCanvas& canvas = mScreen.canvas();
	canvas.add( std::make_unique< UiLabel >( kGameTitle, UiScreen::headingPosition(), UI_TEXT_LARGE, UI_ALIGN_CENTER, kTitleColor ) );
	canvas.add( std::make_unique< UiButton >( UiScreen::menuButtonRect( kStartButtonIndex ), "Start", []() {
		SceneManager::instance().requestChange( SceneManager::kPlayScene );
	} ) );
	canvas.add( std::make_unique< UiButton >( UiScreen::menuButtonRect( kQuitButtonIndex ), "Quit", []() {
		Framework::instance().requestEnd();
	} ) );
}

void TitleScene::beginFrame() {
	// 入力は1回の描画につき1回だけ読む（Main は読まないので、シーンが自分で読む）。
	Pad::instance()->update();
	mScreen.handleInput();
}

void TitleScene::update() {
	// 時間で進むものは無い（ボタンの操作は入力を読んだ beginFrame で済ませる）。
}

void TitleScene::draw() {
	mScreen.draw();
}
