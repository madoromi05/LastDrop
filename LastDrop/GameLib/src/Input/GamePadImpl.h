#ifndef INCLUDED_GAMELIB_INPUT_GAMEPADIMPL_H
#define INCLUDED_GAMELIB_INPUT_GAMEPADIMPL_H

#include <windows.h>
#include <Xinput.h>
#include <cmath>
#undef min
#undef max

#include "Input/GamePadMapping.h"
#include "Input/DirectInputPad.h"

namespace GameLib{
namespace Input{

class GamePad::Impl{
public:
	Impl() :
	mConnected( false ),
	mSourceName( kNoSourceName ),
	mUserIndex( 0 ),
	mDirectInputPad( 0 ),
	mNextRetryTime( 0 ),
	mDoubleBufferIndex( 0 ){
		for ( int i = 0; i < GamePad::BUTTON_MAX; ++i ){
			mButtons[ 0 ][ i ] = false;
			mButtons[ 1 ][ i ] = false;
		}
		clearSticks();
	}
	// 差し口の番号（0〜GamePad::PAD_MAX-1）。配列で作るので、作った後に Manager が渡す。
	void setUserIndex( DWORD userIndex ){
		mUserIndex = userIndex;
	}
	// XInput がつながっていないときに読む DirectInput のパッド（無ければ 0）。持ち主は ManagerImpl。
	void setDirectInputPad( DirectInputPad* pad ){
		mDirectInputPad = pad;
	}
	// active: ウィンドウがアクティブか。違えば何も押していないことにする（キーボード・マウスと同じ）。
	void update( bool active ){
		mDoubleBufferIndex = 1 - mDoubleBufferIndex;
		GamePadRawState raw;
		mConnected = active && read( &raw );
		if ( !mConnected ){
			mSourceName = kNoSourceName;
			for ( int i = 0; i < GamePad::BUTTON_MAX; ++i ){
				mButtons[ mDoubleBufferIndex ][ i ] = false;
			}
			clearSticks();
			return;
		}
		for ( int i = 0; i < GamePad::BUTTON_MAX; ++i ){
			mButtons[ mDoubleBufferIndex ][ i ] = raw.buttons[ i ];
		}
		applyDeadZone( raw.stickX[ GamePad::STICK_LEFT ], raw.stickY[ GamePad::STICK_LEFT ], XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, GamePad::STICK_LEFT );
		applyDeadZone( raw.stickX[ GamePad::STICK_RIGHT ], raw.stickY[ GamePad::STICK_RIGHT ], XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, GamePad::STICK_RIGHT );
	}
	bool isOn( int i ) const {
		ASSERT( i >= 0 && i < GamePad::BUTTON_MAX );
		return mButtons[ mDoubleBufferIndex ][ i ];
	}
	bool isTriggered( int i ) const {
		ASSERT( i >= 0 && i < GamePad::BUTTON_MAX );
		int idx = mDoubleBufferIndex;
		return mButtons[ idx ][ i ] && ( !mButtons[ 1 - idx ][ i ] );
	}
	float stickX( int i ) const {
		ASSERT( i >= 0 && i < GamePad::STICK_MAX );
		return mStickX[ i ];
	}
	float stickY( int i ) const {
		ASSERT( i >= 0 && i < GamePad::STICK_MAX );
		return mStickY[ i ];
	}
	bool mConnected;
	const char* mSourceName; //今読んでいる元（"XInput"・対応表の名前・"None"）
private:
	// XInput を先に読み、つながっていなければ DirectInput のパッドを読む（Xbox 系のパッドは DirectInput にも出てくるが、対応表に無いので二重には読まない）。
	bool read( GamePadRawState* raw ){
		if ( readXInput( raw ) ){
			mSourceName = kXInputSourceName;
			return true;
		}
		if ( mDirectInputPad && mDirectInputPad->read( raw ) ){
			mSourceName = gamePadLayoutName( mDirectInputPad->layout() );
			return true;
		}
		return false;
	}
	// つながっていない差し口を毎回読むと 1 回で数 ms かかることがあるので、
	// 抜けている間は kRetryIntervalMs ごとにだけ確かめる（差した後、最大でこの時間だけ遅れて読み始める）。
	bool readXInput( GamePadRawState* raw ){
		ULONGLONG now = GetTickCount64();
		if ( now < mNextRetryTime ){
			return false;
		}
		XINPUT_STATE state;
		ZeroMemory( &state, sizeof( XINPUT_STATE ) );
		if ( XInputGetState( mUserIndex, &state ) != ERROR_SUCCESS ){
			mNextRetryTime = now + kRetryIntervalMs;
			return false;
		}
		const XINPUT_GAMEPAD& pad = state.Gamepad;
		for ( int i = 0; i < kDigitalButtonCount; ++i ){
			raw->buttons[ i ] = ( pad.wButtons & kButtonMasks[ i ] ) != 0;
		}
		raw->buttons[ GamePad::BUTTON_LT ] = pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		raw->buttons[ GamePad::BUTTON_RT ] = pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
		// 縦は XInput が上を正にしているので、下が正（GamePadRawState の約束）へ反転する。
		raw->stickX[ GamePad::STICK_LEFT ] = static_cast< float >( pad.sThumbLX );
		raw->stickY[ GamePad::STICK_LEFT ] = -static_cast< float >( pad.sThumbLY );
		raw->stickX[ GamePad::STICK_RIGHT ] = static_cast< float >( pad.sThumbRX );
		raw->stickY[ GamePad::STICK_RIGHT ] = -static_cast< float >( pad.sThumbRY );
		return true;
	}
	// 真ん中からの距離で遊びを取る（軸ごとに取ると、斜めに倒したとき遊びの形が四角になり、ななめだけ反応が早くなる）。
	// 遊びの外側は 0〜1 に引き伸ばす（遊びの境目で値が飛ばないように）。遊びの幅は XInput の公式の値を DirectInput のパッドにも使う。
	void applyDeadZone( float x, float y, int deadZone, int stick ){
		float length = std::sqrt( x * x + y * y );
		if ( length <= static_cast< float >( deadZone ) ){
			mStickX[ stick ] = 0.f;
			mStickY[ stick ] = 0.f;
			return;
		}
		float clamped = ( length < kStickRawMax ) ? length : kStickRawMax;
		float scale = ( clamped - deadZone ) / ( kStickRawMax - deadZone ) / length;
		mStickX[ stick ] = x * scale;
		mStickY[ stick ] = y * scale;
	}
	void clearSticks(){
		for ( int i = 0; i < GamePad::STICK_MAX; ++i ){
			mStickX[ i ] = 0.f;
			mStickY[ i ] = 0.f;
		}
	}
	void operator=( const Impl& ); //禁止

	static constexpr const char* kNoSourceName = "None";
	static constexpr const char* kXInputSourceName = "XInput";
	static const ULONGLONG kRetryIntervalMs = 1000;
	// GamePad::Button の A〜十字キー右（BUTTON_DPAD_RIGHT まで）と wButtons のビットの対応。LT・RT はトリガーの深さで決める。
	static const int kDigitalButtonCount = GamePad::BUTTON_DPAD_RIGHT + 1;
	static constexpr WORD kButtonMasks[ kDigitalButtonCount ] = {
		XINPUT_GAMEPAD_A,
		XINPUT_GAMEPAD_B,
		XINPUT_GAMEPAD_X,
		XINPUT_GAMEPAD_Y,
		XINPUT_GAMEPAD_LEFT_SHOULDER,
		XINPUT_GAMEPAD_RIGHT_SHOULDER,
		XINPUT_GAMEPAD_BACK,
		XINPUT_GAMEPAD_START,
		XINPUT_GAMEPAD_LEFT_THUMB,
		XINPUT_GAMEPAD_RIGHT_THUMB,
		XINPUT_GAMEPAD_DPAD_UP,
		XINPUT_GAMEPAD_DPAD_DOWN,
		XINPUT_GAMEPAD_DPAD_LEFT,
		XINPUT_GAMEPAD_DPAD_RIGHT,
	};

	DWORD mUserIndex;
	DirectInputPad* mDirectInputPad; //借りているだけ（持ち主は ManagerImpl）
	ULONGLONG mNextRetryTime; // XInput が抜けている間、次に確かめてよい時刻（GetTickCount64 の ms）
	int mDoubleBufferIndex;
	bool mButtons[ 2 ][ GamePad::BUTTON_MAX ];
	float mStickX[ GamePad::STICK_MAX ];
	float mStickY[ GamePad::STICK_MAX ];
};

} //namespace Input
} //namespace GameLib

#endif
