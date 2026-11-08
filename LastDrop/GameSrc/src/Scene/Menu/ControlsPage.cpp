#include "GameSrc/Scene/Menu/ControlsPage.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/Widget/UiLabel.h"
#include "GameSrc/UI/Widget/UiButton.h"
#include "GameSrc/Render/VertexColor.h"

#include "GameLib/GameLib.h"
#include <memory>
#include <string>
#include <vector>

using namespace GameLib::Input;

namespace{
	const char* kHeading = "操作の一覧";
	const unsigned kHeadingColor = VertexColor::WHITE;
	const unsigned kActionColor = VertexColor::pack( 0.9f, 0.9f, 0.9f );
	const unsigned kKeyColor = VertexColor::pack( 0.85f, 0.65f, 0.15f ); // ボタンを選んだときと同じ燃料の色（黄）
	const char* kBackLabel = "戻る";
	const int kPlayer = 0;          // 一覧に見せるのは自キャラを動かす 0 番の割り当て
	const int kActionColumn = 0;    // UiScreen::listLinePosition の列
	const int kKeyColumn = 1;
	const char* kWorkSeparator = " ";       // 1 行に複数の操作があるとき（移動の W A S D）
	const char* kBindingSeparator = " / ";  // 1 つの操作に複数のキーがあるとき

	// 一覧の 1 行。works の操作に割り当てたキーを、順に並べて見せる。
	struct Line{
		const char* action;
		std::vector< WorkId > works;
	};

	// 上から見せる順。移動は W A S D と読めるよう、上・左・下・右の順に並べる。
	const Line kLines[] = {
		{ "移動",           { WorkId::Up, WorkId::Left, WorkId::Down, WorkId::Right } },
		{ "走る",            { WorkId::Run } },
		{ "見回す",           { WorkId::Look } },
		{ "撃つ",           { WorkId::Fire } },
		{ "覗く",            { WorkId::Aim } },
		{ "銃を出す／しまう", { WorkId::Holster } },
		{ "リロード",         { WorkId::Reload } },
		{ "ライト",     { WorkId::Flashlight } },
		{ "拾う・使う",       { WorkId::Interact } },
		{ "カバン",      { WorkId::Inventory } },
		{ "一時停止",          { WorkId::Menu } },
	};
	const int kLineCount = static_cast< int >( sizeof( kLines ) / sizeof( kLines[ 0 ] ) );

	// パッド（DirectInput のジョイスティック・GamePad）の割り当てか。
	bool isPadDevice( DeviceKind device ) {
		return device == DeviceKind::Joystick || device == DeviceKind::GamePadButton || device == DeviceKind::GamePadAxis;
	}

	// work に割り当てたキーとマウスの名前（パッドはこの一覧では見せない。PC のキーとマウスで遊ぶ前提）。
	std::string keyNames( WorkId work ) {
		std::string names;
		for ( const BindingRecord& record : Pad::instance()->bindings( kPlayer ) ) {
			if ( record.work != work || isPadDevice( record.device ) ) {
				continue;
			}
			if ( !names.empty() ) {
				names += kBindingSeparator;
			}
			names += BindingDisplayName( record );
		}
		return names;
	}

	std::string lineKeys( const Line& line ) {
		std::string keys;
		for ( WorkId work : line.works ) {
			if ( !keys.empty() ) {
				keys += kWorkSeparator;
			}
			keys += keyNames( work );
		}
		return keys;
	}
}

void ControlsPage::build( UiCanvas* canvas, std::function< void() > back ) {
	ASSERT( canvas && back && "ControlsPage : canvas is null or back is empty." );
	canvas->add( std::make_unique< UiLabel >( kHeading, UI_ANCHOR_CENTER, UiScreen::headingPosition(), UI_TEXT_LARGE, UI_ALIGN_CENTER, kHeadingColor ) );
	for ( int i = 0; i < kLineCount; ++i ) {
		canvas->add( std::make_unique< UiLabel >( kLines[ i ].action, UI_ANCHOR_CENTER, UiScreen::listLinePosition( i, kActionColumn ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, kActionColor ) );
		std::string keys = lineKeys( kLines[ i ] );
		canvas->add( std::make_unique< UiLabel >( keys.c_str(), UI_ANCHOR_CENTER, UiScreen::listLinePosition( i, kKeyColumn ), UI_TEXT_NORMAL, UI_ALIGN_MIDDLE_LEFT, kKeyColor ) );
	}
	canvas->add( std::make_unique< UiButton >( UI_ANCHOR_CENTER, UiScreen::listButtonRect( kLineCount ), kBackLabel, back ) );
}
