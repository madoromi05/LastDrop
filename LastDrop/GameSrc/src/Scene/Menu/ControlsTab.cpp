#include "GameSrc/Scene/Menu/ControlsTab.h"
#include "GameSrc/Core/Pad.h"
#include "GameSrc/UI/UiCanvas.h"
#include "GameSrc/UI/UiScreen.h"
#include "GameSrc/UI/Widget/UiKeyBindingGroup.h"
#include "GameSrc/UI/Widget/UiTabBar.h"

#include "GameLib/GameLib.h"
#include <memory>
#include <string>
#include <vector>

using namespace GameLib::Input;

namespace{
	const int kPlayer = 0;          // 一覧に見せるのは自キャラを動かす 0 番の割り当て

	// どの機器の割り当てを見せるか。小さい切り替えの行の、左からの順
	enum Device{
		DEVICE_KEYBOARD_MOUSE,
		DEVICE_PAD,
	};
	const std::vector< std::string > kDeviceLabels = { "キーボード・マウス", "パッド" };	// Device の順
	const float kDeviceTabWidth = 130.f;	// 切り替えのタブ 1 つの幅（長い方の「キーボード・マウス」と、次の名前までの間）

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

	// 移動は W A S D と読めるよう、上・左・下・右の順に並べる。パッドの移動と見回すはスティック 1 本（Move・LookStick）
	// キーボードとパッドで操作の名前（WorkId）が違う行は、両方を並べておく。見せるのは切り替えで選んだ機器の割り当てだけ
		const std::vector< Group > kGroups = {
			{ "移動", {
				{ "移動", { WorkId::Up, WorkId::Left, WorkId::Down, WorkId::Right, WorkId::Move } },
				{ "走る", { WorkId::Run } },
				{ "見回す", { WorkId::Look, WorkId::LookStick } },
			} },
			{ "戦う", {
				{ "撃つ", { WorkId::Fire } },
				{ "覗く", { WorkId::Aim } },
				{ "リロード", { WorkId::Reload } },
			} },
			{ "そのほか", {
				{ "ライト", { WorkId::Flashlight } },
				{ "拾う・使う", { WorkId::Interact } },
				{ "カバン", { WorkId::Inventory } },
				{ "一時停止", { WorkId::Menu } },
			} },
	};

	// パッド（DirectInput のジョイスティック・GamePad）の割り当てか
	bool isPadDevice( DeviceKind device ) {
		return device == DeviceKind::Joystick || device == DeviceKind::GamePadButton || device == DeviceKind::GamePadAxis;
	}

	// record を device の一覧に見せるときのキーの名前。その一覧に見せない割り当てなら空
	std::string keyNameOf( const BindingRecord& record, Device device ) {
		if (device == DEVICE_PAD) {
			return GamePadBindingDisplayName( record );	// パッドの割り当てでなければ空
		}
		return isPadDevice( record.device ) ? std::string() : BindingDisplayName( record );
	}

	// line の行。device の割り当ての名前を 1 つずつ並べる
	UiKeyBindingGroup::Row rowOf( const Line& line, Device device ) {
		UiKeyBindingGroup::Row row;
		row.action = line.action;
		for (WorkId work : line.works) {
			for (const BindingRecord& record : Pad::instance()->bindings( kPlayer )) {
				if (record.work != work) {
					continue;
				}
				std::string name = keyNameOf( record, device );
				if (!name.empty()) {
					row.keys.push_back( name );
				}
			}
		}
		return row;
	}

	std::vector< UiKeyBindingGroup::Row > rowsOf( const Group& group, Device device ) {
		std::vector< UiKeyBindingGroup::Row > rows;
		for (const Line& line : group.lines) {
			rows.push_back( rowOf( line, device ) );
		}
		return rows;
	}
}

void ControlsTab::build( UiCanvas* canvas ) {
	ASSERT( canvas && "ControlsTab : canvas is null." );
	const int groupCount = static_cast< int >( kGroups.size() );
	std::vector< UiKeyBindingGroup* > groups;
	for (int i = 0; i < groupCount; ++i) {
		groups.push_back( canvas->add( std::make_unique< UiKeyBindingGroup >( UiScreen::controlsColumn( i, groupCount ), UiScreen::controlsGroupTop(), kGroups[i].name, rowsOf( kGroups[i], DEVICE_KEYBOARD_MOUSE ) ) ) );
	}
	canvas->add( std::make_unique< UiTabBar >( UiScreen::sideSubTabBarRect(), kDeviceLabels, DEVICE_KEYBOARD_MOUSE, kDeviceTabWidth, UI_TEXT_CAPTION,
		[ groups ]( int device ) {	// まとまりは同じ canvas が持つので、切り替えの行から借りて使い続けてよい
			for (size_t i = 0; i < groups.size(); ++i) {
				groups[i]->setRows( rowsOf( kGroups[i], static_cast< Device >( device ) ) );
			}
		} ) );
}
