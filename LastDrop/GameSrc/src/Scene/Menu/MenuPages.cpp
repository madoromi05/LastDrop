#include "GameSrc/Scene/Menu/MenuPages.h"
#include "GameSrc/Scene/Menu/OptionsPage.h"
#include "GameSrc/Scene/Menu/ControlsPage.h"
#include "GameSrc/Core/GameSettings.h"
#include "GameSrc/UI/UiScreen.h"

#include "GameLib/GameLib.h"

MenuPages::MenuPages( UiRenderer* renderer, GameSettings* settings, AudioEngine* audio, const MenuDefinition& main ) :
	mRenderer( renderer ),
	mSettings( settings ),
	mAudio( audio ),
	mMain( main ),
	mPage( MENU_PAGE_MAIN ),
	mRequested( MENU_PAGE_NONE ) {
	ASSERT( mRenderer && mSettings && mAudio && "MenuPages : renderer, settings or audio is null." );
	open( MENU_PAGE_MAIN );
}

MenuPages::~MenuPages() {
	if ( mPage != MENU_PAGE_MAIN ) {
		mSettings->save();
	}
}

void MenuPages::handleInput() {
	if ( mRequested != MENU_PAGE_NONE ) {
		open( mRequested );
		mRequested = MENU_PAGE_NONE;
	}
	mScreen->handleInput();
}

void MenuPages::draw() {
	mScreen->draw();
}

bool MenuPages::back() {
	// 予約がまだ済んでいなければ、予約したページから 1 つ上へ（ESC を続けて押したとき）
	MenuPageId current = ( mRequested != MENU_PAGE_NONE ) ? mRequested : mPage;
	switch ( current ) {
	case MENU_PAGE_CONTROLS:
		request( MENU_PAGE_OPTIONS ); // 操作の一覧は設定から開くので、設定へ戻る
		return true;
	case MENU_PAGE_OPTIONS:
		request( MENU_PAGE_MAIN );
		return true;
	default:
		return false;
	}
}

void MenuPages::showMain() {
	mRequested = MENU_PAGE_NONE;
	if ( mPage != MENU_PAGE_MAIN ) {
		open( MENU_PAGE_MAIN );
	}
}

void MenuPages::request( MenuPageId page ) {
	mRequested = page;
}

void MenuPages::open( MenuPageId page ) {
	// 設定のページから最初のページへ戻るときに書く（値を変えるたびに書くと、ドラッグの間に何度も書くことになる）
	if ( mPage != MENU_PAGE_MAIN && page == MENU_PAGE_MAIN ) {
		mSettings->save();
	}
	mPage = page;
	mScreen = std::make_unique< UiScreen >( mRenderer );
	auto openPage = [ this ]( MenuPageId next ) { request( next ); };
	auto goBack = [ this ]() { back(); };
	switch ( page ) {
	case MENU_PAGE_OPTIONS:
		OptionsPage::build( &mScreen->canvas(), mSettings, mAudio, openPage, goBack );
		break;
	case MENU_PAGE_CONTROLS:
		ControlsPage::build( &mScreen->canvas(), goBack );
		break;
	default:
		MenuLayout::build( &mScreen->canvas(), mMain, openPage );
		break;
	}
}
