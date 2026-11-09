#ifndef INCLUDED_MENU_PAGES_H
#define INCLUDED_MENU_PAGES_H

#include "GameSrc/Scene/Menu/MenuLayout.h"
#include <memory>

class UiScreen;
class UiRenderer;
class AudioEngine;
struct GameSettings;

// メニューの「今のページ」（最初のページ・設定・操作の一覧）を持ち、ページを切り替えて出す。
// タイトル（MenuScene）と本編の一時停止（PauseMenu）が同じ物を持つので、設定画面への行き方・戻り方は 1 か所。
// ・ページは 1 枚ずつ作り直す（UiCanvas は部品を外せないので、切り替えるたびに UiScreen ごと作る）。
// ・切り替えは予約にして、次の handleInput の頭で行う。押したボタンを持っている画面を、その処理の途中で消さないため
//   （SceneManager::requestChange と同じ考え方）。
// ・設定のページを閉じて最初のページへ戻るとき、設定をファイルへ書く。
// SummaryPlan/個別仕様/UI/OptionsMenu_Plan.txt。
class MenuPages{
public:
	// renderer / settings / audio: 借りる。所有権は受け取らない。nullptr 不可。これより長く生きていること。
	// main: 最初のページの中身。写して持つので、渡した後は消してよい。
	MenuPages( UiRenderer* renderer, GameSettings* settings, AudioEngine* audio, const MenuDefinition& main );
	// 設定のページを開いたまま消える（ウィンドウを閉じた）ときも、変えた設定を書く。
	~MenuPages();
	MenuPages( const MenuPages& ) = delete;
	MenuPages& operator=( const MenuPages& ) = delete;

	// 予約されたページの切り替えをしてから、今のページの入力を読む。1回の描画につき1回。
	void handleInput();
	// 今のページを描く。1回の描画につき1回。中で UiRenderer::draw まで行う。
	void draw();
	// 1 つ上のページへ戻る（予約。ESC）。もう最初のページなら何もせず false を返す（呼ぶ側が一時停止を閉じるなど）。
	bool back();
	// 最初のページへすぐ戻す（一時停止を開き直したとき）。ボタンの処理の中からは呼ばないこと（押したボタンが消える）。
	void showMain();

private:
	void request( MenuPageId page );
	void open( MenuPageId page ); // 今のページを捨てて page を作る

	UiRenderer* mRenderer;  // 借りているだけ
	GameSettings* mSettings; // 借りているだけ
	AudioEngine* mAudio;    // 借りているだけ
	MenuDefinition mMain;
	std::unique_ptr< UiScreen > mScreen;
	MenuPageId mPage;       // 今のページ（MENU_PAGE_NONE にはならない）
	MenuPageId mRequested;  // 予約されたページ。予約が無ければ MENU_PAGE_NONE
};

#endif
