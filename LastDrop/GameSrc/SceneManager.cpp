#include "SceneManager.h"
#include "IScene.h"
#include "SkinTestScene.h"
#include "GameLib/GameLib.h"

SceneManager::SceneManager() :
	mCurrent( 0 ) {
}

SceneManager::~SceneManager() {
	destroy();
}

SceneManager& SceneManager::instance() {
	static SceneManager s;
	return s;
}

void SceneManager::change( SceneId id ) {
	// 種別から実体を生成して切り替える。
	// シーンを増やすときは SceneId(SceneManager.h) とこの分岐の2箇所だけ直せばよい。
	switch ( id ) {
	case SCENE_SKIN_TEST:
		change( static_cast< IScene* >( NEW SkinTestScene() ) );
		break;
	case SCENE_GAME:
	default:
		// 実体を持たせず current を空にする。Main 側で従来ゲームがフォールバック実行される。
		change( static_cast< IScene* >( 0 ) );
		break;
	}
}

void SceneManager::change( IScene* next ) {
	if ( mCurrent != next ) {
		SAFE_DELETE( mCurrent );
		mCurrent = next;
	}
}

void SceneManager::update() {
	if ( mCurrent ) {
		mCurrent->update();
	}
}

void SceneManager::destroy() {
	SAFE_DELETE( mCurrent );
}

IScene* SceneManager::current() const {
	return mCurrent;
}
