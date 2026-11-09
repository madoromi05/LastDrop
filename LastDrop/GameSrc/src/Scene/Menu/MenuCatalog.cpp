#include "GameSrc/Scene/Menu/MenuCatalog.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Render/VertexColor.h"
#include "Framework.h"

using namespace GameLib;

namespace{
	const char* kGameTitle = "ZOMBIE ESCAPE"; // 仮の名前
	const unsigned kTitleColor = VertexColor::pack( 0.95f, 0.75f, 0.2f );
	const char* kPauseHeading = "Pause";
	const unsigned kPauseColor = VertexColor::WHITE;

	// 押されたら種別 id のシーンへ切り替える処理。ボタンの処理の途中でシーンを消さないよう、予約にする（SceneManager::requestChange）。
	std::function< void() > changeTo( SceneManager::SceneId id ) {
		return [ id ]() { SceneManager::instance().requestChange( id ); };
	}
}

MenuDefinition MenuCatalog::title() {
	MenuDefinition menu;
	menu.heading = kGameTitle;
	menu.headingColor = kTitleColor;
	menu.buttons.push_back( { "Start", changeTo( SceneManager::SCENE_GAME ) } ); // すぐ町へ（1 回勝負。SimpleRun_Plan.txt）
	menu.buttons.push_back( { "Options", nullptr, MENU_PAGE_OPTIONS } );
	menu.buttons.push_back( { "Quit", []() { Framework::instance().requestEnd(); } } );
	return menu;
}

MenuDefinition MenuCatalog::pause( std::function< void() > onResume ) {
	MenuDefinition menu;
	menu.heading = kPauseHeading;
	menu.headingColor = kPauseColor;
	menu.buttons.push_back( { "Resume", onResume } );
	menu.buttons.push_back( { "Options", nullptr, MENU_PAGE_OPTIONS } );
	// 新しく始め直す（条件・ガソリンの箱・車の場所・箱の中身を全部引き直す。SimpleRun_Plan.txt C1）。残る物は無いので、失う物も無い。
	menu.buttons.push_back( { "Retry", changeTo( SceneManager::SCENE_GAME ) } );
	menu.buttons.push_back( { "Title", changeTo( SceneManager::SCENE_TITLE ) } );
	menu.buttons.push_back( { "Quit", []() { Framework::instance().requestEnd(); } } );
	return menu;
}
