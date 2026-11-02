#include "GameLib/GameLib.h"
#include "Input/GamePadMapping.h"

namespace GameLib{
namespace Input{

namespace{
	// DIJOYSTATE2 の軸。
	enum DirectInputAxis{
		AXIS_X,
		AXIS_Y,
		AXIS_Z,
		AXIS_RX,
		AXIS_RY,
		AXIS_RZ,
	};
	// 対応表のボタンの欄に入れる、ボタン番号ではない値。
	const int kNoButton = -1; // このパッドには無い
	const int kPovButton = -2; // 十字キー（POV の向きから決める）

	// POV の向き（1/100 度。上が 0 で時計回り）。斜めは両方の向きを押したことにする。
	const DWORD kPovCentered = 0xffff; // 下位 16 ビットがこれなら、何も押していない
	const DWORD kPovUpRightEdge = 4500;
	const DWORD kPovDownRightEdge = 13500;
	const DWORD kPovDownLeftEdge = 22500;
	const DWORD kPovUpLeftEdge = 31500;
	const BYTE kButtonPressedBit = 0x80; // DIJOYSTATE2::rgbButtons は上位ビットが押下
}

struct GamePadLayout{
	const char* name;
	int buttons[ GamePad::BUTTON_MAX ];      // GamePad::Button の順に、DirectInput のボタン番号（kNoButton / kPovButton も入る）
	DirectInputAxis stickX[ GamePad::STICK_MAX ]; // GamePad::Stick の順に、横の軸
	DirectInputAxis stickY[ GamePad::STICK_MAX ]; // 縦の軸（DirectInput は下が正なので、そのまま使う）
};

namespace{
	// Sony のパッド（DualSense・DualShock 4）。どちらも DirectInput では同じ並び。
	// 2026-11-02 DualSense（Bluetooth）でボタン 14・軸 6（X Y Z Rx Ry Rz）・POV 1 を確認。
	// ボタンの番号は DualShock 4 / DualSense の一般的な並び（実機で押して確かめる。F6 の Pad の欄）。
	// PS ボタン（12）とタッチパッドの押し込み（13）は使わない。L2/R2 は押し込むとオンになるボタン（6・7）で読み、軸（Rx・Ry）は使わない。
	const GamePadLayout kSonyLayout = {
		"Sony (DualSense / DualShock 4)",
		{
			1,           // BUTTON_A ← ✕
			2,           // BUTTON_B ← ○
			0,           // BUTTON_X ← □
			3,           // BUTTON_Y ← △
			4,           // BUTTON_LB ← L1
			5,           // BUTTON_RB ← R1
			8,           // BUTTON_BACK ← Create（DualShock 4 は Share）
			9,           // BUTTON_START ← Options
			10,          // BUTTON_LEFT_THUMB ← L3
			11,          // BUTTON_RIGHT_THUMB ← R3
			kPovButton,  // BUTTON_DPAD_UP
			kPovButton,  // BUTTON_DPAD_DOWN
			kPovButton,  // BUTTON_DPAD_LEFT
			kPovButton,  // BUTTON_DPAD_RIGHT
			6,           // BUTTON_LT ← L2
			7,           // BUTTON_RT ← R2
		},
		{ AXIS_X, AXIS_Z },  // 左スティック X・右スティック Z
		{ AXIS_Y, AXIS_RZ }, // 左スティック Y・右スティック Rz
	};

	struct GamePadProduct{
		WORD vendorId;
		WORD productId;
		const GamePadLayout* layout;
	};
	const WORD kSonyVendorId = 0x054c;
	const GamePadProduct kProducts[] = {
		{ kSonyVendorId, 0x0ce6, &kSonyLayout }, // DualSense
		{ kSonyVendorId, 0x0df2, &kSonyLayout }, // DualSense Edge
		{ kSonyVendorId, 0x05c4, &kSonyLayout }, // DualShock 4（初代）
		{ kSonyVendorId, 0x09cc, &kSonyLayout }, // DualShock 4（2 代目）
	};

	float axisValue( const DIJOYSTATE2& state, DirectInputAxis axis ){
		switch ( axis ){
			case AXIS_X: return static_cast< float >( state.lX );
			case AXIS_Y: return static_cast< float >( state.lY );
			case AXIS_Z: return static_cast< float >( state.lZ );
			case AXIS_RX: return static_cast< float >( state.lRx );
			case AXIS_RY: return static_cast< float >( state.lRy );
			case AXIS_RZ: return static_cast< float >( state.lRz );
		}
		return 0.f;
	}

	// 十字キーの 1 つの向きが押されているか（POV の角度から）。
	bool isPovOn( DWORD pov, GamePad::Button button ){
		if ( ( pov & kPovCentered ) == kPovCentered ){
			return false;
		}
		switch ( button ){
			case GamePad::BUTTON_DPAD_UP: return pov >= kPovUpLeftEdge || pov <= kPovUpRightEdge;
			case GamePad::BUTTON_DPAD_RIGHT: return pov >= kPovUpRightEdge && pov <= kPovDownRightEdge;
			case GamePad::BUTTON_DPAD_DOWN: return pov >= kPovDownRightEdge && pov <= kPovDownLeftEdge;
			case GamePad::BUTTON_DPAD_LEFT: return pov >= kPovDownLeftEdge && pov <= kPovUpLeftEdge;
			default: return false;
		}
	}
}

const GamePadLayout* findGamePadLayout( DWORD productGuidData1 ){
	WORD vendorId = LOWORD( productGuidData1 );
	WORD productId = HIWORD( productGuidData1 );
	for ( const GamePadProduct& product : kProducts ){
		if ( product.vendorId == vendorId && product.productId == productId ){
			return product.layout;
		}
	}
	return 0;
}

const char* gamePadLayoutName( const GamePadLayout* layout ){
	return layout->name;
}

void applyGamePadLayout( const GamePadLayout* layout, const DIJOYSTATE2& state, GamePadRawState* out ){
	for ( int i = 0; i < GamePad::BUTTON_MAX; ++i ){
		int code = layout->buttons[ i ];
		if ( code == kPovButton ){
			out->buttons[ i ] = isPovOn( state.rgdwPOV[ 0 ], static_cast< GamePad::Button >( i ) );
		}else if ( code == kNoButton ){
			out->buttons[ i ] = false;
		}else{
			out->buttons[ i ] = ( state.rgbButtons[ code ] & kButtonPressedBit ) != 0;
		}
	}
	for ( int i = 0; i < GamePad::STICK_MAX; ++i ){
		out->stickX[ i ] = axisValue( state, layout->stickX[ i ] );
		out->stickY[ i ] = axisValue( state, layout->stickY[ i ] );
	}
}

} //namespace Input
} //namespace GameLib
