#include "GameSrc/Scene/Base/RunEndScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Raid/RunState.h"
#include "GameSrc/Raid/RaidParams.h"
#include "GameSrc/UI/UiLabel.h"
#include "GameSrc/UI/UiButton.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include <cmath>
#include <cstdio>
#include <memory>

using namespace GameLib::Math;

namespace{
	const int kTextSize = 64;

	const float kHeadingTop = 60.f;   // 見出しの真ん中
	const float kDayTop = 100.f;      // 終わった日の行の真ん中
	const float kFirstLineTop = 150.f; // 内訳の 1 行目の真ん中
	const float kLineHeight = 30.f;
	// 内訳の列の左端（横の中央から）。名前・掛け算・点。
	const float kNameColumn = -200.f;
	const float kDetailColumn = 0.f;
	const float kPointsColumn = 130.f;
	// 内訳の行（上から）。合計の前に 1 行空ける。
	enum ScoreLine{
		LINE_DAYS,
		LINE_KILLS,
		LINE_ITEMS,
		LINE_CLEAR_BONUS,
		LINE_GAP,
		LINE_TOTAL,
		LINE_BEST,
	};
	const UiRect kTitleRect = { -110.f, 400.f, 220.f, 44.f }; // 横の中央からのずれ・上端・幅・高さ（メニューのボタンと同じ大きさ）

	const char* kClearFormat = "%d 日間 生き延びた";
	const char* kKilledHeading = "死亡";
	const char* kMissingHeading = "行方不明";
	const char* kDayFormat = "DAY %d";
	const char* kTimesFormat = "%d × %d";
	const char* kNoDetail = "";
	const char* kNewBest = "最高記録";
	const char* kBestFormat = "最高 %d";
	const char* kTitleLabel = "タイトルへ";

	const unsigned kClearColor = VertexColor::pack( 0.4f, 0.9f, 0.4f );
	const unsigned kFailColor = VertexColor::pack( 0.85f, 0.12f, 0.1f );
	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );
	const unsigned kHintColor = VertexColor::pack( 0.65f, 0.65f, 0.65f );
	const unsigned kTotalColor = VertexColor::pack( 0.95f, 0.85f, 0.3f );

	float lineTop( int index ) {
		return kFirstLineTop + index * kLineHeight;
	}

	int roundToInt( float value ) {
		return static_cast< int >( std::lround( value ) );
	}
}

RunEndScene::RunEndScene( GameResources* resources ) :
	mScreen( &resources->uiRenderer() ) {
	GameLib::Input::Manager::instance().setCursorLocked( false ); // 本編でロックしたカーソルを戻す
	RunState& run = resources->run();
	run.setRunEndPending( false ); // 見せたので、次の 1 日の結果の Continue は拠点へ
	const RunEnd& end = run.lastRunEnd();
	const Score& score = end.score;
	const RaidParams& params = raidParams();
	char text[ kTextSize ];
	UiCanvas& canvas = mScreen.canvas();

	bool cleared = ( end.ending == RaidEnding::Survived );
	const char* heading = kKilledHeading;
	if ( cleared ) {
		std::snprintf( text, sizeof( text ), kClearFormat, RunState::kDayCount );
		heading = text;
	} else if ( end.ending == RaidEnding::Missing ) {
		heading = kMissingHeading;
	}
	canvas.add( std::make_unique< UiLabel >( heading, UI_ANCHOR_CENTER, Vector2( 0.f, kHeadingTop ), UI_TEXT_LARGE, UI_ALIGN_CENTER,
		cleared ? kClearColor : kFailColor ) );
	std::snprintf( text, sizeof( text ), kDayFormat, end.day );
	canvas.add( std::make_unique< UiLabel >( text, UI_ANCHOR_CENTER, Vector2( 0.f, kDayTop ), UI_TEXT_NORMAL, UI_ALIGN_CENTER, kHintColor ) );

	// 内訳（R2_Plan.txt 6-4）。掛け算の重みは今の調整値（RaidParams）で出す。
	std::snprintf( text, sizeof( text ), kTimesFormat, score.daysSurvived, roundToInt( params.scorePerDay ) );
	addScoreLine( LINE_DAYS, "生き延びた日数", text, score.dayPoints, kTextColor );
	std::snprintf( text, sizeof( text ), kTimesFormat, score.kills, roundToInt( params.scorePerKill ) );
	addScoreLine( LINE_KILLS, "倒した数", text, score.killPoints, kTextColor );
	addScoreLine( LINE_ITEMS, "持ち帰った物", kNoDetail, score.itemPoints, kTextColor );
	addScoreLine( LINE_CLEAR_BONUS, "クリアのボーナス", kNoDetail, score.clearBonus, kTextColor );
	addScoreLine( LINE_TOTAL, "スコア", kNoDetail, score.total(), kTotalColor );
	if ( end.newBest ) {
		canvas.add( std::make_unique< UiLabel >( kNewBest, UI_ANCHOR_CENTER, Vector2( kPointsColumn, lineTop( LINE_BEST ) ), UI_TEXT_NORMAL,
			UI_ALIGN_MIDDLE_LEFT, kTotalColor ) );
	} else {
		std::snprintf( text, sizeof( text ), kBestFormat, run.bestScore() );
		canvas.add( std::make_unique< UiLabel >( text, UI_ANCHOR_CENTER, Vector2( kPointsColumn, lineTop( LINE_BEST ) ), UI_TEXT_NORMAL,
			UI_ALIGN_MIDDLE_LEFT, kHintColor ) );
	}
	canvas.add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, kTitleRect, kTitleLabel, []() {
		SceneManager::instance().requestChange( SceneManager::SCENE_TITLE );
	} ) );
}

void RunEndScene::addScoreLine( int index, const char* name, const char* detail, int points, unsigned color ) {
	char text[ kTextSize ];
	float top = lineTop( index );
	UiCanvas& canvas = mScreen.canvas();
	canvas.add( std::make_unique< UiLabel >( name, UI_ANCHOR_CENTER, Vector2( kNameColumn, top ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, color ) );
	canvas.add( std::make_unique< UiLabel >( detail, UI_ANCHOR_CENTER, Vector2( kDetailColumn, top ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, kHintColor ) );
	std::snprintf( text, sizeof( text ), "%d", points );
	canvas.add( std::make_unique< UiLabel >( text, UI_ANCHOR_CENTER, Vector2( kPointsColumn, top ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, color ) );
}

void RunEndScene::beginFrame() {
	mScreen.handleInput();
}

void RunEndScene::update() {
	// 時間で進むものは無い。
}

void RunEndScene::draw() {
	mScreen.draw();
}
