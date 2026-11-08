#include "GameSrc/Scene/Base/BaseScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Raid/RunStateFile.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Raid/RunRules.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/DebugMode/EditorWindow.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"

namespace{
	const char* kPreparingLabel = "町へ向かっています"; // 本編を作り始める前のロード画面の文字
	const float kNoProgress = 0.f;
}

BaseScene::BaseScene( GameResources* resources ) :
	mResources( resources ),
	mScreen( &resources->uiRenderer() ),
	mLoadingScreen( &resources->uiRenderer() ),
	mDeploying( false ) {
	ASSERT( mResources && "BaseScene : resources is null." );
	// 本編でロックしたカーソルを戻す（マスをマウスで選べるように）。
	GameLib::Input::Manager::instance().setCursorLocked( false );
	EditorWindow::setRun( &mResources->run() ); // F6 の Raid の欄（倉庫に物を足す・日を変える・保存を消す）
}

BaseScene::~BaseScene() {
	// 動かした物を書く（タイトルへ戻る・窓を閉じる・出撃のどれでも。出撃は RunRules::deploy でも書くが、同じ中身なので 2 回でも困らない）。
	RunStateFile::save( mResources->run() );
	EditorWindow::setRun( nullptr );
}

void BaseScene::beginFrame() {
	if ( mDeploying ) {
		// 出撃を押したフレームにロード画面を描いた。このフレームの終わりに本編を作る（作る間もこの画面のまま）。
		SceneManager::instance().requestChange( SceneManager::SCENE_GAME );
		return;
	}
	RunState& run = mResources->run();
	// ESC は持ち上げている物を取りやめる。何もしていなければタイトルへ（動かした物は、このシーンが消えるときに保存する）。
	if ( InputMenu() ) {
		if ( !mScreen.back() ) {
			SceneManager::instance().requestChange( SceneManager::SCENE_TITLE );
		}
		return;
	}
	if ( mScreen.handleInput( &run ) == StashScreen::Request::Deploy ) {
		RunRules::deploy( &run ); // 保存（この形がその日のやり直しの形になる）
		mDeploying = true; // 本編への切り替えは、ロード画面を 1 回描いた次のフレーム
	}
}

void BaseScene::update() {
	// 時間で進むものは無い（操作は入力を読んだ beginFrame で済ませる）。
}

void BaseScene::draw() {
	if ( mDeploying ) {
		mLoadingScreen.draw( mResources->run().day(), kPreparingLabel, kNoProgress );
		return;
	}
	mScreen.draw( mResources->run() );
}
