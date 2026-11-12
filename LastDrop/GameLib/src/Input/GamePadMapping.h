#ifndef INCLUDED_GAMELIB_INPUT_GAMEPADMAPPING_H
#define INCLUDED_GAMELIB_INPUT_GAMEPADMAPPING_H

#include "GameLib/Input/GamePad.h"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#undef min
#undef max

namespace GameLib{
namespace Input{

// パッドの 1 回分の入力を、Xbox の並びに置き換えた形（GamePad::Impl が遊びを取る前）
// XInput から読んでも DirectInput から読んでも、この形にそろえてから使う
// スティックは -kStickRawMax〜kStickRawMax（右・下が正）
struct GamePadRawState{
	bool buttons[ GamePad::BUTTON_MAX ];
	float stickX[ GamePad::STICK_MAX ];
	float stickY[ GamePad::STICK_MAX ];
};

// スティックを倒しきった値（XInput の SHORT の最大。DirectInput の軸の範囲もこれにそろえる）
const float kStickRawMax = 32767.f;

// DirectInput のパッドのボタンと軸の並び（製品ごとの対応表の中身）
struct GamePadLayout;

// 製品（DIDEVICEINSTANCE::guidProduct.Data1。下位 16 ビットが VID・上位が PID）から対応表を引く
// 対応表に無ければ nullptr（そのパッドは GamePad では読まない）
const GamePadLayout* findGamePadLayout( DWORD productGuidData1 );
// 対応表の名前（F6 のエディタで、どの対応表で読んでいるかを見せる）。文字列リテラルなので持ち続けてよい
const char* gamePadLayoutName( const GamePadLayout* layout );
// DirectInput の 1 回分の入力を、対応表で Xbox の並びに置き換える
void applyGamePadLayout( const GamePadLayout* layout, const DIJOYSTATE2& state, GamePadRawState* out );

} //namespace Input
} //namespace GameLib

#endif
