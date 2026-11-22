#include "GameSrc/Scene/Menu/MenuCatalog.h"
#include "GameSrc/Scene/SceneManager.h"
#include "Framework.h"

using namespace GameLib;

namespace{
	const char* kGameTitle = "LAST DROP";
	const char* kTitleBackdrop = "Resource/UI/title_backdrop.tga";
	// 絵の街灯は真ん中より右にあるので、はみ出た横は右を多めに切る（4:3 の画面でも題名と街灯が重ならない）
	const float kTitleBackdropAlignX = 0.25f;
	const char* kPauseHeading = "PAUSE";

	// 押されたら種別 id のシーンへ切り替える処理。ボタンの処理の途中でシーンを消さないよう、予約にする（SceneManager::requestChange）
	std::function< void() > changeTo( SceneManager::SceneId id ) {
		return [ id ]() { SceneManager::instance().requestChange( id ); };
	}
}

MenuDefinition MenuCatalog::title() {
	MenuDefinition menu;
	menu.heading = kGameTitle;
	menu.buttons.push_back( { "START", changeTo( SceneManager::SCENE_GAME ) } ); // すぐ町へ（1 回勝負）
	menu.buttons.push_back( { "OPTIONS", nullptr, MENU_PAGE_OPTIONS } );
	menu.buttons.push_back( { "QUIT", []() { Framework::instance().requestEnd(); } } );
	menu.backdrop = kTitleBackdrop;
	menu.backdropAlignX = kTitleBackdropAlignX;
	menu.music = SOUND_BGM_TITLE;
	return menu;
}

MenuDefinition MenuCatalog::pause( std::function< void() > onResume ) {
	MenuDefinition menu;
	menu.heading = kPauseHeading;
	menu.buttons.push_back( { "RESUME", onResume } );
	menu.buttons.push_back( { "OPTIONS", nullptr, MENU_PAGE_OPTIONS } );
	// 新しく始め直す（条件・ガソリンの箱・車の場所・箱の中身を全部引き直す）。残る物は無いので、失う物も無い
	// 行き先は今の出撃と同じ種類のシーン（ゾンビの確認用のシーンなら、確認用のシーンのまま）
	menu.buttons.push_back( { "RETRY", []() { SceneManager::instance().requestRetryRaid(); } } );
	menu.buttons.push_back( { "TITLE", changeTo( SceneManager::SCENE_TITLE ) } );
	menu.buttons.push_back( { "QUIT", []() { Framework::instance().requestEnd(); } } );
	return menu;
}
