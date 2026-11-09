#include "GameSrc/Scene/Menu/MenuCatalog.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Render/VertexColor.h"
#include "Framework.h"

using namespace GameLib;

namespace{
	const char* kGameTitle = "ZOMBIE ESCAPE"; // 仮の名前
	const unsigned kTitleColor = VertexColor::pack( 0.95f, 0.75f, 0.2f ); // MENU_LOOK_SIDE では使わない（色は MenuPalette）
	const char* kTitleBackdrop = "Resource/UI/title_backdrop.tga";
	// 絵の街灯は真ん中より右にあるので、はみ出た横は右を多めに切る（4:3 の画面でも題名と街灯が重ならない）。
	const float kTitleBackdropAlignX = 0.25f;
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
	// 文字は大文字（RE:2 風の行は字の間を広げた大文字で並べる。一時停止のボタンは変えない）。
	menu.buttons.push_back( { "START", changeTo( SceneManager::SCENE_GAME ) } ); // すぐ町へ（1 回勝負）
	menu.buttons.push_back( { "OPTIONS", nullptr, MENU_PAGE_OPTIONS } );
	menu.buttons.push_back( { "QUIT", []() { Framework::instance().requestEnd(); } } );
	menu.look = MENU_LOOK_SIDE;
	menu.backdrop = kTitleBackdrop;
	menu.backdropAlignX = kTitleBackdropAlignX;
	return menu;
}

MenuDefinition MenuCatalog::pause( std::function< void() > onResume ) {
	MenuDefinition menu;
	menu.heading = kPauseHeading;
	menu.headingColor = kPauseColor;
	menu.buttons.push_back( { "Resume", onResume } );
	menu.buttons.push_back( { "Options", nullptr, MENU_PAGE_OPTIONS } );
	// 新しく始め直す（条件・ガソリンの箱・車の場所・箱の中身を全部引き直す）。残る物は無いので、失う物も無い。
	menu.buttons.push_back( { "Retry", changeTo( SceneManager::SCENE_GAME ) } );
	menu.buttons.push_back( { "Title", changeTo( SceneManager::SCENE_TITLE ) } );
	menu.buttons.push_back( { "Quit", []() { Framework::instance().requestEnd(); } } );
	return menu;
}
