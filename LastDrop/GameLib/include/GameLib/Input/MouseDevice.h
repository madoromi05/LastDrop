#pragma once
#include "InputDeviceBase.h"
#include "GameLib/Input/Manager.h"
#include "GameLib/Input/Mouse.h"
using namespace GameLib::Input;

// ────────────────────────────────────────────────
// マウスボタン用フォーマット
//
// KeyCodeType = Mouse::Button
// ────────────────────────────────────────────────
class MouseButtonFormat : public InputButtonFormatBase<Mouse::Button>
{
public:
    bool CheckButtonState(Mouse::Button a_keyCode) override
    {
        return Manager::instance().mouse().isOn(a_keyCode);
    }
};

// ────────────────────────────────────────────────
// マウス軸（移動量）用フォーマット
//
// KeyCodeType = int（軸の種類を番号で区別する）
//   0 : マウス移動（velocityX, velocityY）
//   1 : ホイール（wheel を width に入れる）
// ────────────────────────────────────────────────
class MouseAxisFormat : public InputAxisFormatBase<int>
{
public:
    AxisStatus CheckAxisState(int a_keyCode) override
    {
        Mouse m = Manager::instance().mouse();
        if (a_keyCode == 0)
        {
            // マウスの移動速度を軸として返す
            return AxisStatus(
                static_cast<float>(m.velocityX()),
                static_cast<float>(m.velocityY())
            );
        }
        if (a_keyCode == 1)
        {
            return AxisStatus(static_cast<float>(m.wheel()), 0.f);	// ホイールを横軸として返す（縦は0）
        }
        return AxisStatus();
    }
};

// ────────────────────────────────────────────────
// マウスデバイス
// ────────────────────────────────────────────────
class MouseDevice : public InputDeviceBase<int>
{
    // ボタンと軸で KeyCodeType が違うため、
    // ボタン用には MouseButtonFormat（Mouse::Button）を直接登録するのではなく、
    // ボタンコードを int に統一したラッパーを使う
};

// ────────────────────────────────────────────────
// マウスボタンを int コードで扱うフォーマット（デバイス統一用）
// Mouse::Button を int にキャストして保持する
// ────────────────────────────────────────────────
class MouseButtonFormatInt : public InputButtonFormatBase<int>
{
public:
    bool CheckButtonState(int a_keyCode) override
    {
        return Manager::instance().mouse().isOn(
            static_cast<Mouse::Button>(a_keyCode)
        );
    }
};