#include "Framework.h"
using namespace GameLib;

#include "GameSrc/Core/Pad.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/DebugMode/EditorWindow.h"
#include "GameLib/DebugUi/DebugUi.h"
#include "GameLib/Profiling/Profiler.h"

// 起動時に開くシーン。種類の一覧は SceneManager::SceneId（SceneManager.h）を参照
const SceneManager::SceneId kStartScene = SceneManager::SCENE_TITLE;


bool gInitialized = false;

namespace GameLib{

void Framework::update(){
	if ( !gInitialized ){
		Pad::create();
		SceneManager::instance().start( kStartScene );
		gInitialized = true;
	}

	// 入力を読み、今のシーンを更新して描く
	SceneManager::instance().update();
	// 開発用のエディタ（F6）。シーンを描いた後に積み、Framework::postUpdate で一番上に描かれる
	if ( DebugUi::isOpen() ){
		PROFILE_SCOPE( "Editor" );
		EditorWindow::draw();
	}

	if ( isEndRequested() ){
		SceneManager::instance().destroy();
		Pad::destroy();
	}
}
}
