#include "GameSrc/Scene/Base/DayResultScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Raid/RunState.h"
#include "GameSrc/UI/UiLabel.h"
#include "GameSrc/UI/UiButton.h"
#include "GameSrc/UI/UiAnchor.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include <cstdio>
#include <memory>

using namespace GameLib::Math;

namespace{
	const float kHalf = 0.5f;
	const int kNone = -1;
	const int kTextSize = 64;
	const int kSecondsPerMinute = 60;

	const float kHeadingTop = 70.f;     // 見出しの真ん中
	const float kInfoTop = 115.f;       // 時間・倒した数の行の真ん中
	const float kItemsLabelTop = 155.f; // 「Brought back」の行の真ん中
	const float kGridTop = 175.f;       // 持ち帰った物のマス目の上端
	const float kCellSize = 48.f;
	const int kShownColumns = 8;        // 持ち帰った物を横に並べる数（カバン 8 マスが 1 行に入る）
	const UiRect kContinueRect = { -110.f, 400.f, 220.f, 44.f }; // 横の中央からのずれ・上端・幅・高さ（メニューのボタンと同じ大きさ）

	const char* kHeadingFormat = "DAY %d SURVIVED";
	const char* kInfoFormat = "Time %02d:%02d    Kills %d";
	const char* kItemsLabel = "Brought back";
	const char* kNothing = "Nothing";
	const char* kContinueLabel = "Continue";

	const unsigned kHeadingColor = VertexColor::pack( 0.4f, 0.9f, 0.4f );
	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );
	const unsigned kHintColor = VertexColor::pack( 0.65f, 0.65f, 0.65f );
}

DayResultScene::DayResultScene( GameResources* resources ) :
	mScreen( &resources->uiRenderer() ),
	mView( &resources->uiRenderer(), &mIcons ),
	mShown( shownGrid( resources->run().lastDay() ) ),
	mHasItems( !resources->run().lastDay().broughtBack.empty() ) {
	GameLib::Input::Manager::instance().setCursorLocked( false ); // 本編でロックしたカーソルを戻す
	const DayResult& result = resources->run().lastDay();
	char text[ kTextSize ];
	UiCanvas& canvas = mScreen.canvas();
	std::snprintf( text, sizeof( text ), kHeadingFormat, result.day );
	canvas.add( std::make_unique< UiLabel >( text, UI_ANCHOR_CENTER, Vector2( 0.f, kHeadingTop ), UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor ) );
	int seconds = static_cast< int >( result.seconds );
	std::snprintf( text, sizeof( text ), kInfoFormat, seconds / kSecondsPerMinute, seconds % kSecondsPerMinute, result.kills );
	canvas.add( std::make_unique< UiLabel >( text, UI_ANCHOR_CENTER, Vector2( 0.f, kInfoTop ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor ) );
	canvas.add( std::make_unique< UiLabel >( kItemsLabel, UI_ANCHOR_CENTER, Vector2( 0.f, kItemsLabelTop ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor ) );
	if ( !mHasItems ) {
		Vector2 nothing( 0.f, kGridTop + kCellSize * kHalf );
		canvas.add( std::make_unique< UiLabel >( kNothing, UI_ANCHOR_CENTER, nothing, UI_TEXT_NORMAL, UI_ALIGN_CENTER, kTextColor ) );
	}
	// 5 日目に脱出したなら、スコアの画面へ。それ以外は次の日の拠点へ。
	RunState* run = &resources->run();
	canvas.add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, kContinueRect, kContinueLabel, [ run ]() {
		SceneManager::instance().requestChange( run->isRunEndPending() ? SceneManager::SCENE_RUN_END : SceneManager::SCENE_BASE );
	} ) );
}

ItemGrid DayResultScene::shownGrid( const DayResult& result ) {
	int cells = 0;
	for ( const ItemStack& stack : result.broughtBack ) {
		cells += itemSpec( stack.item ).cellSize;
	}
	// 2 マスの物が行の端で入らないこともあるので、全部入るまで行を足す。
	int rows = ( cells + kShownColumns - 1 ) / kShownColumns;
	rows = ( rows < 1 ) ? 1 : rows;
	for ( ;; ++rows ) {
		ItemGrid grid( kShownColumns, rows );
		bool placedAll = true;
		for ( const ItemStack& stack : result.broughtBack ) {
			placedAll = placedAll && grid.placeAnywhere( stack );
		}
		if ( placedAll ) {
			return grid;
		}
	}
}

void DayResultScene::beginFrame() {
	mScreen.handleInput();
}

void DayResultScene::update() {
	// 時間で進むものは無い。
}

void DayResultScene::draw() {
	if ( mHasItems ) {
		float left = UiLayout::anchorX( UI_ANCHOR_CENTER ) - ItemGridView::width( mShown, kCellSize ) * kHalf;
		mView.setLayout( Vector2( left, kGridTop ), kCellSize );
		mView.drawCells( mShown, kNone ); // 積むだけ。すぐ下の画面の描画でまとめて描かれる
	}
	mScreen.draw();
}
