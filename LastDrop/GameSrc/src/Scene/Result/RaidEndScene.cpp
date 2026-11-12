#include "GameSrc/Scene/Result/RaidEndScene.h"
#include "GameSrc/Scene/Result/EscapedPage.h"
#include "GameSrc/Scene/Result/GameOverPage.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Raid/RaidRecord.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include <functional>

namespace{
	std::function< void() > changeTo( SceneManager::SceneId id ) {
		return [ id ]() { SceneManager::instance().requestChange( id ); };	// 行の処理の途中でシーンを消さないよう、予約にする（SceneManager::requestChange）
	}
}

RaidEndScene::RaidEndScene( GameResources* resources ) :
	mScreen( &resources->uiRenderer() ) {
	GameLib::Input::Manager::instance().setCursorLocked( false ); // 本編でロックしたカーソルを戻す
	const RaidResult& result = resources->raidRecord().lastResult();
	ASSERT( result.ending != RaidEnding::None && "RaidEndScene : the raid has not ended." );
	if ( result.ending == RaidEnding::Escaped ) {
		EscapedPage::build( &mScreen.canvas(), result, changeTo( SceneManager::SCENE_TITLE ) );
	} else {
		GameOverPage::build( &mScreen.canvas(), result, changeTo( SceneManager::SCENE_GAME ), changeTo( SceneManager::SCENE_TITLE ) );
	}
}

void RaidEndScene::beginFrame() {
	mScreen.handleInput();
}

void RaidEndScene::update() {
	// 時間で進むものは無い
}

void RaidEndScene::draw() {
	mScreen.draw();
}
