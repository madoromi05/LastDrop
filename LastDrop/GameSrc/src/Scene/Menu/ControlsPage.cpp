#include "GameSrc/Scene/Menu/ControlsPage.h"
#include "GameSrc/Scene/Menu/MenuLayout.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/Widget/UiKeyBindingGroup.h"
#include "GameSrc/UI/Widget/UiMenuItem.h"
#include "GameSrc/UI/Widget/UiKeyHintRow.h"

#include "GameLib/GameLib.h"
#include <memory>
#include <string>
#include <vector>

using namespace GameLib::Input;

namespace{
	const char* kHeading = "操作の一覧";
	const char* kBackLabel = "戻る";
	const int kPlayer = 0;          // 一覧に見せるのは自キャラを動かす 0 番の割り当て

	// 一覧の 1 行。works の操作に割り当てたキーを、順に並べて見せる
	struct Line{
		const char* action;
		std::vector< WorkId > works;
	};

	// まとまり 1 つ。左の列から順に 1 列ずつ使う
	struct Group {
		const char* name;
		std::vector< Line > lines;
	};

	// 移動は W A S D と読めるよう、上・左・下・右の順に並べる
		const std::vector< Group > kGroups = {
			{ "移動", {
				{ "移動", { WorkId::Up, WorkId::Left, WorkId::Down, WorkId::Right } },
				{ "走る", { WorkId::Run } },
				{ "見回す", { WorkId::Look } },
			} },
			{ "戦う", {
				{ "撃つ", { WorkId::Fire } },
				{ "覗く", { WorkId::Aim } },
				{ "銃を出す／しまう", { WorkId::Holster } },
				{ "リロード", { WorkId::Reload } },
			} },
			{ "そのほか", {
				{ "ライト", { WorkId::Flashlight } },
				{ "拾う・使う", { WorkId::Interact } },
				{ "カバン", { WorkId::Inventory } },
				{ "一時停止", { WorkId::Menu } },
			} },
	};

	// 画面の下の操作の案内。ESC で設定へ戻る（MenuPages）
	const std::vector< UiKeyHintRow::Hint > kKeyHints = {
		{ "Esc", "戻る" },
	};

	// パッド（DirectInput のジョイスティック・GamePad）の割り当てか
	bool isPadDevice( DeviceKind device ) {
		return device == DeviceKind::Joystick || device == DeviceKind::GamePadButton || device == DeviceKind::GamePadAxis;
	}

	// line の行。キーとマウスの名前を 1 つずつ並べる（パッドは見せない。PC のキーとマウスで遊ぶ前提）
		UiKeyBindingGroup::Row rowOf( const Line & line ) {
		UiKeyBindingGroup::Row row;
		row.action = line.action;
		for (WorkId work : line.works) {
			for (const BindingRecord& record : Pad::instance()->bindings( kPlayer )) {
				if (record.work == work && !isPadDevice( record.device )) {
					row.keys.push_back( BindingDisplayName( record ) );
				}
			}
		}
		return row;
	}

	std::vector< UiKeyBindingGroup::Row > rowsOf( const Group& group ) {
		std::vector< UiKeyBindingGroup::Row > rows;
		for (const Line& line : group.lines) {
			rows.push_back( rowOf( line ) );
		}
		return rows;
	}
}

void ControlsPage::build( UiCanvas* canvas, std::function< void() > back ) {
	ASSERT( canvas && back && "ControlsPage : canvas is null or back is empty." );
	MenuLayout::buildSideHeading( canvas, kHeading );
	const int groupCount = static_cast< int >( kGroups.size() );
	for (int i = 0; i < groupCount; ++i) {
		canvas->add( std::make_unique< UiKeyBindingGroup >( UiScreen::controlsColumn( i, groupCount ), UiScreen::controlsGroupTop(), kGroups[i].name, rowsOf( kGroups[i] ) ) );
	}
	canvas->add( std::make_unique< UiMenuItem >( UI_ANCHOR_LEFT, UiScreen::sideBackItemRect(), kBackLabel, back, UiMenuItem::kJapaneseSpacing ) );
	canvas->add( std::make_unique< UiKeyHintRow >( UI_ANCHOR_LEFT, UiScreen::keyHintPosition(), kKeyHints ) );
}
