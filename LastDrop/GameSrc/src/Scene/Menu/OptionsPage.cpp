#include "GameSrc/Scene/Menu/OptionsPage.h"
#include "GameSrc/Core/GameSettings.h"
#include "GameSrc/Audio/AudioEngine.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/Widget/UiLabel.h"
#include "GameSrc/UI/Widget/UiButton.h"
#include "GameSrc/UI/Widget/UiSlider.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include <cstdio>
#include <memory>
#include <string>

namespace{
	const char* kHeading = "設定";
	const unsigned kHeadingColor = VertexColor::WHITE;
	const char* kMasterVolumeLabel = "全体の音量";
	const char* kEffectVolumeLabel = "効果音の音量";
	const char* kSensitivityLabel = "マウス感度";
	const char* kControlsLabel = "操作の一覧";
	const char* kBackLabel = "戻る";

	// 上から並べる行の番号（UiScreen::settingRowRect / settingButtonRect の番号）。
	enum Row{
		ROW_MASTER_VOLUME,
		ROW_EFFECT_VOLUME,
		ROW_SENSITIVITY,
		ROW_CONTROLS,
		ROW_BACK,
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
		return std::make_unique< UiSlider >( UI_ANCHOR_CENTER, UiScreen::settingRowRect( row ), label, kVolumeRange,
			[ volume ]() { return *volume; },
			[ volume, settings, audio ]( int value ) { *volume = value; settings->applyAudio( *audio ); },
			formatVolume );
	}
}

void OptionsPage::build( UiCanvas* canvas, GameSettings* settings, AudioEngine* audio,
	std::function< void( MenuPageId ) > openPage, std::function< void() > back ) {
	ASSERT( canvas && settings && audio && "OptionsPage : canvas, settings or audio is null." );
	ASSERT( openPage && back && "OptionsPage : openPage or back is empty." );
	canvas->add( std::make_unique< UiLabel >( kHeading, UI_ANCHOR_CENTER, UiScreen::headingPosition(), UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor ) );
	canvas->add( makeVolumeSlider( ROW_MASTER_VOLUME, kMasterVolumeLabel, &settings->masterVolume, settings, audio ) );
	canvas->add( makeVolumeSlider( ROW_EFFECT_VOLUME, kEffectVolumeLabel, &settings->effectVolume, settings, audio ) );
	// 感度は自キャラ（FirstPersonControl）が毎回読むので、値を書き換えるだけでよい。
	canvas->add( std::make_unique< UiSlider >( UI_ANCHOR_CENTER, UiScreen::settingRowRect( ROW_SENSITIVITY ), kSensitivityLabel, kSensitivityRange,
		[ settings ]() { return settings->mouseSensitivity; },
		[ settings ]( int value ) { settings->mouseSensitivity = value; },
		formatSensitivity ) );
	canvas->add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, UiScreen::settingButtonRect( ROW_CONTROLS ), kControlsLabel,
		[ openPage ]() { openPage( MENU_PAGE_CONTROLS ); } ) );
	canvas->add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, UiScreen::settingButtonRect( ROW_BACK ), kBackLabel, back ) );
}
