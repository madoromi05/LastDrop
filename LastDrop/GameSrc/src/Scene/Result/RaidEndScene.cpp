#include "GameSrc/Scene/Result/RaidEndScene.h"
#include "GameSrc/Scene/GameResources.h"
#include "GameSrc/Scene/SceneManager.h"
#include "GameSrc/Raid/RaidRecord.h"
#include "GameSrc/UI/Widget/UiLabel.h"
#include "GameSrc/UI/Widget/UiButton.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include "GameLib/Input/Manager.h"
#include <cmath>
#include <cstdio>
#include <memory>

using namespace GameLib::Math;

namespace{
	const int kTextSize = 64;
	const int kSecondsPerMinute = 60;

	const float kHeadingTop = 90.f;    // 見出しの真ん中
	const float kFirstLineTop = 170.f; // 1 行目の真ん中
	const float kLineHeight = 34.f;
	// 行の列の左端（横の中央から）。名前・値。
	const float kNameColumn = -150.f;
	const float kValueColumn = 60.f;
	// 行（上から）。
	enum ResultLine{
		LINE_TIME,
		LINE_KILLS,
	};
	// ボタン（横の中央からのずれ・上端・幅・高さ。メニューのボタンと同じ大きさ）。死んだときは上に「もう一度」を足す。
	const UiRect kRetryRect = { -110.f, 340.f, 220.f, 44.f };
	const UiRect kTitleRect = { -110.f, 400.f, 220.f, 44.f };

	// 見出しは英語（タイトル・一時停止のボタンと同じ）。中の文字とボタンは日本語。
	const char* kEscapedHeading = "ESCAPED";
	const char* kKilledHeading = "KILLED IN ACTION";
	const char* kTimeName = "かかった時間";
	const char* kKillsName = "倒した数";
	const char* kTimeFormat = "%d:%02d";
	const char* kKillsFormat = "%d";
	const char* kRetryLabel = "もう一度";
	const char* kTitleLabel = "タイトルへ";

	const unsigned kEscapedColor = VertexColor::pack( 0.4f, 0.9f, 0.4f );
	const unsigned kKilledColor = VertexColor::pack( 0.85f, 0.12f, 0.1f );
	const unsigned kTextColor = VertexColor::pack( 0.92f, 0.92f, 0.92f );

	float lineTop( int index ) {
		return kFirstLineTop + index * kLineHeight;
	}

	std::function< void() > changeTo( SceneManager::SceneId id ) {
		// ボタンの処理の途中でシーンを消さないよう、予約にする（SceneManager::requestChange）。
		return [ id ]() { SceneManager::instance().requestChange( id ); };
	}
}

RaidEndScene::RaidEndScene( GameResources* resources ) :
	mScreen( &resources->uiRenderer() ) {
	GameLib::Input::Manager::instance().setCursorLocked( false ); // 本編でロックしたカーソルを戻す
	const RaidResult& result = resources->raidRecord().lastResult();
	ASSERT( result.ending != RaidEnding::None && "RaidEndScene : the raid has not ended." );
	bool escaped = ( result.ending == RaidEnding::Escaped );
	UiCanvas& canvas = mScreen.canvas();
	canvas.add( std::make_unique< UiLabel >( escaped ? kEscapedHeading : kKilledHeading, UI_ANCHOR_CENTER, Vector2( 0.f, kHeadingTop ),
		UI_TEXT_LARGE, UI_ALIGN_CENTER, escaped ? kEscapedColor : kKilledColor ) );

	char text[ kTextSize ];
	int seconds = static_cast< int >( std::floor( result.seconds ) ); // 経った秒は切り捨て（0:59.9 は 0:59）
	std::snprintf( text, sizeof( text ), kTimeFormat, seconds / kSecondsPerMinute, seconds % kSecondsPerMinute );
	addLine( LINE_TIME, kTimeName, text );
	std::snprintf( text, sizeof( text ), kKillsFormat, result.kills );
	addLine( LINE_KILLS, kKillsName, text );

	// ボタンは上から足す（上下キーで選ぶ順。始めは一番上が選ばれる）。
	if ( !escaped ) {
		canvas.add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, kRetryRect, kRetryLabel, changeTo( SceneManager::SCENE_GAME ) ) );
	}
	canvas.add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, kTitleRect, kTitleLabel, changeTo( SceneManager::SCENE_TITLE ) ) );
}

void RaidEndScene::addLine( int index, const char* name, const char* value ) {
	float top = lineTop( index );
	UiCanvas& canvas = mScreen.canvas();
	canvas.add( std::make_unique< UiLabel >( name, UI_ANCHOR_CENTER, Vector2( kNameColumn, top ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, kTextColor ) );
	canvas.add( std::make_unique< UiLabel >( value, UI_ANCHOR_CENTER, Vector2( kValueColumn, top ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, kTextColor ) );
}

void RaidEndScene::beginFrame() {
	mScreen.handleInput();
}

void RaidEndScene::update() {
	// 時間で進むものは無い。
}

void RaidEndScene::draw() {
	mScreen.draw();
}
