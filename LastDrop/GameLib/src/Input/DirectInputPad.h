#ifndef INCLUDED_GAMELIB_INPUT_DIRECTINPUTPAD_H
#define INCLUDED_GAMELIB_INPUT_DIRECTINPUTPAD_H

#include "Input/GamePadMapping.h"

namespace GameLib{
namespace Input{

// DirectInput のパッド 1 台（対応表 GamePadLayout にある物だけ）。読んだ入力を Xbox の並びにして GamePad::Impl へ渡す。
// Joystick::Impl と違い、作るのに失敗しても止めない（create が 0 を返し、そのパッドを飛ばす）。
// 他のアプリ（Steam など）と取り合わないよう、DISCL_NONEXCLUSIVE で読む。
class DirectInputPad{
public:
	// device の持ち主になる（失敗したときも Release する）。layout は対応表の物（持ち主にはならない）。
	static DirectInputPad* create( IDirectInputDevice8* device, const GamePadLayout* layout, HWND windowHandle ){
		HRESULT hr = device->SetDataFormat( &c_dfDIJoystick2 );
		if ( SUCCEEDED( hr ) ){
			// ウィンドウが手前にある間だけ読む（後ろにある間は GetDeviceState が失敗し、何も押していないことになる）。
			hr = device->SetCooperativeLevel( windowHandle, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE );
		}
		if ( SUCCEEDED( hr ) ){
			hr = setAxisRange( device );
		}
		if ( FAILED( hr ) ){
			device->Release();
			return 0;
		}
		return NEW DirectInputPad( device, layout );
	}
	~DirectInputPad(){
		mDevice->Unacquire();
		mDevice->Release();
		mDevice = 0;
	}
	// 今の入力を読む。読めなければ false（抜けた・ウィンドウが後ろにある）。
	bool read( GamePadRawState* out ){
		// 取れていなくても毎回頼み直す（Bluetooth が切れてつなぎ直したときも、同じ機器ならここで戻る）。
		mDevice->Acquire();
		mDevice->Poll(); //Poll が要らない機器では何もしない
		DIJOYSTATE2 state;
		if ( FAILED( mDevice->GetDeviceState( sizeof( DIJOYSTATE2 ), &state ) ) ){
			return false;
		}
		applyGamePadLayout( mLayout, state, out );
		return true;
	}
	const GamePadLayout* layout() const {
		return mLayout;
	}
private:
	DirectInputPad( IDirectInputDevice8* device, const GamePadLayout* layout ) :
	mDevice( device ),
	mLayout( layout ){
	}
	// 全部の軸を -kStickRawMax〜kStickRawMax にする（何もしないと 0〜65535 で、XInput と遊びの取り方を共通にできない）。
	static HRESULT setAxisRange( IDirectInputDevice8* device ){
		DIPROPRANGE range;
		range.diph.dwSize = sizeof( DIPROPRANGE );
		range.diph.dwHeaderSize = sizeof( DIPROPHEADER );
		range.diph.dwHow = DIPH_DEVICE;
		range.diph.dwObj = 0;
		range.lMin = -static_cast< LONG >( kStickRawMax );
		range.lMax = static_cast< LONG >( kStickRawMax );
		return device->SetProperty( DIPROP_RANGE, &range.diph );
	}
	DirectInputPad( const DirectInputPad& ); //禁止
	void operator=( const DirectInputPad& ); //禁止

	IDirectInputDevice8* mDevice;
	const GamePadLayout* mLayout;
};

} //namespace Input
} //namespace GameLib

#endif
