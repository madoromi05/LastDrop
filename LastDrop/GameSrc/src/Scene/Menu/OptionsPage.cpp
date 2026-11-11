#include "GameSrc/Scene/Menu/OptionsPage.h"
#include "GameSrc/Core/GameSettings.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/Widget/UiSlider.h"
#include "GameSrc/UI/Widget/UiKeyHintRow.h"

#include "GameLib/GameLib.h"
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace{
	const char* kHeading = "設定";
	const char* kMasterVolumeLabel = "全体の音量";
	const char* kEffectVolumeLabel = "効果音の音量";
	const char* kSensitivityLabel = "マウス感度";
	const char* kControlsLabel = "操作の一覧";
	const char* kBackLabel = "戻る";

	// 上から並べるスライダーの行（UiScreen::sideSettingRowRect の番号）。
	enum Row{
		ROW_MASTER_VOLUME,
		ROW_EFFECT_VOLUME,
		ROW_SENSITIVITY,

		ROW_NUMBER,
	};
	// スライダーの下に、少し空けて並べるページを開く行（UiScreen::sideSettingLinkRect の番号）。
	enum Link{
		LINK_CONTROLS,
		LINK_BACK,
	};

	// 画面の下の操作の案内。値は左右で変え、ESC で 1 ページ戻る（UiCanvas・MenuPages）。
	const std::vector< UiKeyHintRow::Hint > kKeyHints = {
		{ "← →", "値を変える" },
		{ "Esc", "戻る" },
	};

	const UiSlider::Range kVolumeRange = { GameSettings::kVolumeMin, GameSettings::kVolumeMax, GameSettings::kVolumeStep };
	const UiSlider::Range kSensitivityRange = { GameSettings::kSensitivityMin, GameSettings::kSensitivityMax, GameSettings::kSensitivityStep };
	const int kPercent = 100;

	// 音量は数のまま（"80"）。
	std::string formatVolume( int value ) {
		return std::to_string( value );
	}

	// 感度は百分率を倍率に直して見せる（125 → "1.25"）。
	std::string formatSensitivity( int value ) {
		const int kBufferSize = 16;
		char buffer[ kBufferSize ];
		std::snprintf( buffer, kBufferSize, "%d.%02d", value / kPercent, value % kPercent );
		return buffer;
	}

	// 音量のスライダー。値を変えたらすぐ audio へ流す（一時停止の間は音を止めているので、聞こえるのは戻ってから）。
	std::unique_ptr< UiSlider > makeVolumeSlider( Row row, const char* label, int* volume, GameSettings* settings, AudioEngine* audio ) {
		return std::make_unique< UiSlider >( UI_ANCHOR_LEFT, UiScreen::sideSettingRowRect( row ), label, kVolumeRange,
			[ volume ]() { return *volume; },
			[ volume, settings, audio ]( int value ) { *volume = value; settings->applyAudio( *audio ); },
			formatVolume );
	}

	// ページを開く行（板なし。日本語なので字の間は詰める）。
	std::unique_ptr< UiMenuItem > makeLink( Link link, const char* label, std::function< void() > onActivate ) {
		return std::make_unique< UiMenuItem >( UI_ANCHOR_LEFT, UiScreen::sideSettingLinkRect( ROW_NUMBER, link ), label, onActivate, UiMenuItem::kJapaneseSpacing );
	}
}

void OptionsPage::build( UiCanvas* canvas, GameSettings* settings, AudioEngine* audio,
	std::function< void( MenuPageId ) > openPage, std::function< void() > back ) {
	ASSERT( canvas && settings && audio && "OptionsPage : canvas, settings or audio is null." );
	ASSERT( openPage && back && "OptionsPage : openPage or back is empty." );
	MenuLayout::buildSideHeading( canvas, kHeading );
	canvas->add( makeVolumeSlider( ROW_MASTER_VOLUME, kMasterVolumeLabel, &settings->masterVolume, settings, audio ) );
	canvas->add( makeVolumeSlider( ROW_EFFECT_VOLUME, kEffectVolumeLabel, &settings->effectVolume, settings, audio ) );
	// 感度は自キャラ（FirstPersonControl）が毎回読むので、値を書き換えるだけでよい。
	canvas->add( std::make_unique< UiSlider >( UI_ANCHOR_LEFT, UiScreen::sideSettingRowRect( ROW_SENSITIVITY ), kSensitivityLabel, kSensitivityRange,
		[ settings ]() { return settings->mouseSensitivity; },
		[ settings ]( int value ) { settings->mouseSensitivity = value; },
		formatSensitivity ) );
	canvas->add( makeLink( LINK_CONTROLS, kControlsLabel, [ openPage ]() { openPage( MENU_PAGE_CONTROLS ); } ) );
	canvas->add( makeLink( LINK_BACK, kBackLabel, back ) );
	canvas->add( std::make_unique< UiKeyHintRow >( UI_ANCHOR_LEFT, UiScreen::keyHintPosition(), kKeyHints ) );
}
