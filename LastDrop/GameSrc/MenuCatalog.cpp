#include "MenuCatalog.h"
#include "SceneManager.h"
#include "VertexColor.h"
#include "Framework.h"

using namespace GameLib;

namespace{
	const char* kGameTitle = "ZOMBIE ESCAPE"; // 仮の名前
	const unsigned kTitleColor = VertexColor::pack( 0.95f, 0.75f, 0.2f );
	const char* kClearHeading = "ESCAPED";
	const unsigned kClearColor = VertexColor::pack( 0.4f, 0.9f, 0.4f );

	// 押されたら種別 id のシーンへ切り替える処理。ボタンの処理の途中でシーンを消さないよう、予約にする（SceneManager::requestChange）。
	std::function< void() > changeTo( SceneManager::SceneId id ) {
		return [ id ]() { SceneManager::instance().requestChange( id ); };
	}
}

MenuDefinition MenuCatalog::title() {
	MenuDefinition menu;
	menu.heading = kGameTitle;
	menu.headingColor = kTitleColor;
	menu.buttons.push_back( { "Start", changeTo( SceneManager::SCENE_GAME ) } );
	menu.buttons.push_back( { "Quit", []() { Framework::instance().requestEnd(); } } );
	return menu;
}

MenuDefinition MenuCatalog::clear() {
	MenuDefinition menu;
	menu.heading = kClearHeading;
	menu.headingColor = kClearColor;
	menu.buttons.push_back( { "Retry", changeTo( SceneManager::SCENE_GAME ) } );
	menu.buttons.push_back( { "Title", changeTo( SceneManager::SCENE_TITLE ) } );
	return menu;
}
