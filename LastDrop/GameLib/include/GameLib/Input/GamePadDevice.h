#pragma once
#include "InputDeviceBase.h"
#include "InputEnums.h"
#include "Manager.h"
#include "GamePad.h"
using namespace GameLib::Input;

// ────────────────────────────────────────────────
// パッド（GamePad）のボタン用フォーマット
//
// a_keyCode = GamePad::Button の番号。
// 読むパッドは差し口 0 番（JoystickButtonFormat と同じく、プレイヤーごとの差し口の指定は今後の拡張ポイント）。
// ────────────────────────────────────────────────
class GamePadButtonFormat : public InputButtonFormatBase<int>
{
public:
    explicit GamePadButtonFormat(int a_padIndex = 0)
        : m_padIndex(a_padIndex) {}

    bool CheckButtonState(int a_keyCode) override
    {
        if (a_keyCode < 0 || a_keyCode >= GamePad::BUTTON_MAX) return false;
        return Manager::instance().gamePad(m_padIndex).isOn(static_cast<GamePad::Button>(a_keyCode));
    }

private:
    int m_padIndex;
};

// ────────────────────────────────────────────────
// パッド（GamePad）の軸用フォーマット
//
// a_keyCode = GamePadAxisKind（左スティック・右スティック）。
// 値は -1〜1（右・下が正）。遊び（デッドゾーン）は GamePad 側で取ってある。
// ────────────────────────────────────────────────
class GamePadAxisFormat : public InputAxisFormatBase<int>
{
public:
    explicit GamePadAxisFormat(int a_padIndex = 0)
        : m_padIndex(a_padIndex) {}

    AxisStatus CheckAxisState(int a_keyCode) override
    {
        if (a_keyCode < 0 || a_keyCode >= GamePad::STICK_MAX) return AxisStatus();
        GamePad pad = Manager::instance().gamePad(m_padIndex);
        GamePad::Stick stick = static_cast<GamePad::Stick>(a_keyCode);
        return AxisStatus(pad.stickX(stick), pad.stickY(stick));
    }

private:
    int m_padIndex;
};

// ────────────────────────────────────────────────
// パッド（GamePad）デバイス
// ────────────────────────────────────────────────
class GamePadDevice : public InputDeviceBase<int>
{
    // 特に追加実装なし
};
