#include "Framework.h"
using namespace GameLib;

#include "Pad.h"
#include "SceneManager.h"

// 起動時に開くシーン。種類の一覧は SceneManager::SceneId（SceneManager.h）を参照。
const SceneManager::SceneId kStartScene = SceneManager::SCENE_FPS_TEST;

bool gInitialized = false;

namespace GameLib{

void Framework::update(){
	if ( !gInitialized ){
		Pad::create();
		SceneManager::instance().change( kStartScene );
		gInitialized = true;
	}

	// 入力の読み取り・更新・描画は各シーンが行う。
	SceneManager::instance().update();

	if ( isEndRequested() ){
		SceneManager::instance().destroy();
		Pad::destroy();
	}
}
}
