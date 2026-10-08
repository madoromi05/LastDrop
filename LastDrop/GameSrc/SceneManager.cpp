#include "SceneManager.h"
#include "IScene.h"
#include "SkinTestScene.h"
#include "TpsTestScene.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "ClearScene.h"
#include "GameLib/GameLib.h"

SceneManager::SceneManager() :
	mCurrent( 0 ),
	mHasRequest( false ),
	mRequestedId( SCENE_TITLE ) {
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
	case SCENE_FPS_TEST:
		change( static_cast< IScene* >( NEW TpsTestScene() ) );
		break;
	case SCENE_TITLE:
		change( static_cast< IScene* >( NEW TitleScene() ) );
		break;
	case SCENE_GAME:
		change( static_cast< IScene* >( NEW GameScene() ) );
		break;
	case SCENE_CLEAR:
		change( static_cast< IScene* >( NEW ClearScene() ) );
		break;
	default:
		ASSERT( false && "SceneManager::change : unknown SceneId." );
		break;
	}
}

void SceneManager::requestChange( SceneId id ) {
	mHasRequest = true;
	mRequestedId = id;
}

void SceneManager::change( IScene* next ) {
	if ( mCurrent != next ) {
		SAFE_DELETE( mCurrent );
		mCurrent = next;
		mClock.reset(); // 前のシーンで経った時間を、新しいシーンの更新に持ち込まない
	}
}

void SceneManager::update() {
	if ( !mCurrent ) {
		return;
	}
	mCurrent->beginFrame();
	int ticks = mClock.ticksToRun();
	for ( int i = 0; i < ticks; ++i ) {
		mCurrent->update();
	}
	mCurrent->draw();
	// 予約された切り替えは、今のシーンの処理が全部終わってから行う（シーンの途中で自分自身を消さないように）。
	if ( mHasRequest ) {
		mHasRequest = false;
		change( mRequestedId );
	}
}

void SceneManager::destroy() {
	SAFE_DELETE( mCurrent );
}

IScene* SceneManager::current() const {
	return mCurrent;
}
