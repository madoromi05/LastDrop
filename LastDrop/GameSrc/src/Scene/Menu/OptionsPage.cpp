#include "GameSrc/Scene/Menu/OptionsPage.h"
#include "GameSrc/Scene/Menu/ControlsTab.h"
#include "GameSrc/Scene/Menu/MenuLayout.h"
#include "GameSrc/Core/GameSettings.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/Widget/UiSlider.h"
#include "GameSrc/UI/Widget/UiTabBar.h"
#include "GameSrc/UI/Widget/UiKeyHintRow.h"

#include "GameLib/GameLib.h"
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace{
	const char* kHeading = "設定";
	const std::vector< std::string > kTabLabels = { "音声", "感度", "操作" };	// OptionsTab の順
	const float kTabWidth = 72.f;	// タブ 1 つの幅（2 文字の名前と、次の名前までの間）
	const char* kMasterVolumeLabel = "全体の音量";
	const char* kEffectVolumeLabel = "効果音の音量";
	const char* kMouseSensitivityLabel = "マウス感度";
	const char* kPadSensitivityLabel = "パッド感度";
	const char* kBackLabel = "戻る";

	// 音声のタブのスライダーの行（UiScreen::sideSettingRowRect の番号）
	enum AudioRow{
		AUDIO_ROW_MASTER,
		AUDIO_ROW_EFFECT,
	};
	// 感度のタブのスライダーの行（UiScreen::sideSettingRowRect の番号）
	enum SensitivityRow{
		SENSITIVITY_ROW_MOUSE,
		SENSITIVITY_ROW_PAD,
	};

	// 画面の下の操作の案内。左右はタブの行ではタブを、スライダーでは値を変える。ESC で最初のページへ戻る（UiCanvas・MenuPages）
	const std::vector< UiKeyHintRow::Hint > kSliderKeyHints = {
		{ "← →", "タブ・値を変える" },
		{ "Esc", "戻る" },
	};
	const std::vector< UiKeyHintRow::Hint > kControlsKeyHints = {
		{ "← →", "タブを変える" },
		{ "Esc", "戻る" },
	};

	const UiSlider::Range kVolumeRange = { GameSettings::kVolumeMin, GameSettings::kVolumeMax, GameSettings::kVolumeStep };
	const UiSlider::Range kSensitivityRange = { GameSettings::kSensitivityMin, GameSettings::kSensitivityMax, GameSettings::kSensitivityStep };
	const int kPercent = 100;

	// 音量は数のまま（"80"）
	std::string formatVolume( int value ) {
		return std::to_string( value );
	}

	// 感度は百分率を倍率に直して見せる（125 → "1.25"）
	std::string formatSensitivity( int value ) {
		const int kBufferSize = 16;
		char buffer[ kBufferSize ];
		std::snprintf( buffer, kBufferSize, "%d.%02d", value / kPercent, value % kPercent );
		return buffer;
	}

	// 音量のスライダー。値を変えたらすぐ audio へ流す（一時停止の間は音を止めているので、聞こえるのは戻ってから）
	std::unique_ptr< UiSlider > makeVolumeSlider( AudioRow row, const char* label, int* volume, GameSettings* settings, AudioEngine* audio ) {
		return std::make_unique< UiSlider >( UI_ANCHOR_LEFT, UiScreen::sideSettingRowRect( row ), label, kVolumeRange,
			[ volume ]() { return *volume; },
			[ volume, settings, audio ]( int value ) { *volume = value; settings->applyAudio( *audio ); },
			formatVolume );
	}

	// 感度のスライダー。自キャラ（FirstPersonControl）が毎回読むので、値を書き換えるだけでよい
	std::unique_ptr< UiSlider > makeSensitivitySlider( SensitivityRow row, const char* label, int* sensitivity ) {
		return std::make_unique< UiSlider >( UI_ANCHOR_LEFT, UiScreen::sideSettingRowRect( row ), label, kSensitivityRange,
			[ sensitivity ]() { return *sensitivity; },
			[ sensitivity ]( int value ) { *sensitivity = value; },
			formatSensitivity );
	}

	void buildAudioTab( UiCanvas* canvas, GameSettings* settings, AudioEngine* audio ) {
		canvas->add( makeVolumeSlider( AUDIO_ROW_MASTER, kMasterVolumeLabel, &settings->masterVolume, settings, audio ) );
		canvas->add( makeVolumeSlider( AUDIO_ROW_EFFECT, kEffectVolumeLabel, &settings->effectVolume, settings, audio ) );
	}

	void buildSensitivityTab( UiCanvas* canvas, GameSettings* settings ) {
		canvas->add( makeSensitivitySlider( SENSITIVITY_ROW_MOUSE, kMouseSensitivityLabel, &settings->mouseSensitivity ) );
		canvas->add( makeSensitivitySlider( SENSITIVITY_ROW_PAD, kPadSensitivityLabel, &settings->padSensitivity ) );
	}
}

void OptionsPage::build( UiCanvas* canvas, GameSettings* settings, AudioEngine* audio, OptionsTab tab,
	std::function< void( OptionsTab ) > selectTab, std::function< void() > back ) {
	ASSERT( canvas && settings && audio && "OptionsPage : canvas, settings or audio is null." );
	ASSERT( selectTab && back && "OptionsPage : selectTab or back is empty." );
	ASSERT( static_cast< int >( kTabLabels.size() ) == OPTIONS_TAB_NUMBER && "OptionsPage : tab labels do not match OptionsTab." );
	MenuLayout::buildSideHeading( canvas, kHeading );
	// タブの行を最初に足す。タブを変えてページを作り直したときも、選んでいる行がタブの行になり、左右を続けて押せる
	canvas->add( std::make_unique< UiTabBar >( UiScreen::sideTabBarRect(), kTabLabels, tab, kTabWidth, UI_TEXT_MENU,
		[ selectTab ]( int index ) { selectTab( static_cast< OptionsTab >( index ) ); } ) );
	switch ( tab ) {
	case OPTIONS_TAB_AUDIO:
		buildAudioTab( canvas, settings, audio );
		break;
	case OPTIONS_TAB_SENSITIVITY:
		buildSensitivityTab( canvas, settings );
		break;
	default:
		ControlsTab::build( canvas );
		break;
	}
	canvas->add( std::make_unique< UiMenuItem >( UI_ANCHOR_LEFT, UiScreen::sideBackItemRect(), kBackLabel, back, UiMenuItem::kJapaneseSpacing ) );
	canvas->add( std::make_unique< UiKeyHintRow >( UI_ANCHOR_LEFT, UiScreen::keyHintPosition(),
		( tab == OPTIONS_TAB_CONTROLS ) ? kControlsKeyHints : kSliderKeyHints ) );
}
